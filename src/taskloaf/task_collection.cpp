#include "task_collection.hpp"
#include "default_worker.hpp"
#include "comm.hpp"
#include "address.hpp"
#include "logging.hpp"

#include <cereal/types/vector.hpp>

#include <iostream>
#include <random>

namespace taskloaf {

task_collection::task_collection(log& my_log, comm& my_comm):
    my_log(my_log),
    my_comm(my_comm),
    stealing(false)
{}

size_t task_collection::size() const {
    return stealable_tasks.size() + local_tasks.size();
}

void task_collection::add_task(closure t) {
    stealable_tasks.push_front(std::make_pair(next_token, std::move(t)));
    next_token++;
}

void task_collection::add_local_task(closure t) {
    local_tasks.push(std::make_pair(next_token, std::move(t)));
    next_token++;
}

void task_collection::add_task(const address& where, closure t) {
    if (where != my_comm.get_addr()) {
        my_comm.send(where, std::move(t));
    } else {
        add_local_task(std::move(t));
    }
}

void task_collection::run_next() {
    TLASSERT(size() > 0);
    closure t;
    auto grab_local_task = [&] () {
        t = std::move(local_tasks.top().second);
        local_tasks.pop();
    };
    auto grab_stealable_task = [&] () {
        t = std::move(stealable_tasks.front().second);
        stealable_tasks.pop_front();
    };

    if (local_tasks.size() == 0) {
        grab_stealable_task();
    } else if (stealable_tasks.size() == 0) {
        grab_local_task();
    } else if (stealable_tasks.front().first > local_tasks.top().first) {
        grab_stealable_task();
    } else {
        grab_local_task();
    }

    t();
    my_log.n_tasks_run++;
}

void task_collection::steal() {
    if (stealing) {
        return;
    }
    stealing = true;
    my_log.n_attempted_steals++;

    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());

    auto& remotes = my_comm.remote_endpoints();
    if (remotes.size() == 0) {
        return;
    }
    std::uniform_int_distribution<> dis(0, remotes.size() - 1);
    auto idx = dis(gen);

    add_task(remotes[idx], make_closure(
        [] (const address& sender, ignore) {
            auto& tc = ((default_worker*)(cur_worker))->tasks;
            auto response = tc.victimized();

            tc.add_task(sender, make_closure(
                [] (std::vector<closure>& tasks, ignore) {
                    auto& tc = ((default_worker*)(cur_worker))->tasks;
                    tc.receive_tasks(std::move(tasks));
                    return ignore{};
                },
                std::move(response)
            ));
            return ignore{};
        },
        my_comm.get_addr()
    ));
}

std::vector<closure> task_collection::victimized() {
    my_log.n_victimized++;
    auto n_steals = std::min(static_cast<size_t>(1), stealable_tasks.size());
    TLASSERT(n_steals <= stealable_tasks.size());
    std::vector<closure> steals;
    for (size_t i = 0; i < n_steals; i++) {
        steals.push_back(std::move(stealable_tasks.back().second));
        stealable_tasks.pop_back();
    }
    return steals;
}

void task_collection::receive_tasks(std::vector<closure> stolen_tasks) {
    if (stolen_tasks.size() > 0) {
        my_log.n_successful_steals++;
    }
    for (auto& t: stolen_tasks) {
        stealable_tasks.push_back(std::make_pair(next_token, std::move(t)));
        next_token++;
    }
    stealing = false;
}

}//end namespace taskloaf
