#include "catch.hpp"

#include "taskloaf.hpp"
#include "worker_test_helpers.hpp"

using namespace taskloaf;

TEST_CASE("Launch and immediately destroy", "[launch]") {
    launch_local(4);
}

TEST_CASE("Run here") {
    auto ctx = launch_local(4);
    Worker* submit_worker = cur_worker;
    for (size_t i = 0; i < 5; i++) {
        async(Loc::here, [=] () {
            REQUIRE(cur_worker == submit_worker);
        }).wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

TEST_CASE("Number of workers", "[worker]") {
    auto ws = workers(4);
    REQUIRE(ws[0]->n_workers() == 4);
}

struct TestingComm: public Comm {
    Address addr = {0};
    std::vector<Address> remotes = {{1}};
    std::vector<TaskT> sent;

    const Address& get_addr() const { return addr; }
    const std::vector<Address>& remote_endpoints() { return remotes; }
    TaskT recv() { return TaskT(); }

    void send(const Address&, TaskT msg) {
        sent.push_back(std::move(msg));
    }
};

#define MAKE_TASK_COLLECTION(name)\
    Log log({0});\
    TestingComm comm;\
    TaskCollection name(log, comm);\

TEST_CASE("Add task") {
    MAKE_TASK_COLLECTION(tc);
    int x = 0;
    tc.add_task({[&] () { x = 1; }});
    REQUIRE(tc.size() == 1);
    tc.run_next();
    REQUIRE(x == 1);
}

TEST_CASE("Victimized") {
    MAKE_TASK_COLLECTION(tc); 
    tc.add_task({[&] () {}});
    REQUIRE(tc.size() == 1);
    auto tasks = tc.victimized();
    REQUIRE(tc.size() == 0);
    REQUIRE(tasks.size() == 1);
}

TEST_CASE("Receive tasks") {
    MAKE_TASK_COLLECTION(tc); 
    tc.add_task({[&] () {}});
    auto tasks = tc.victimized();
    tc.receive_tasks(std::move(tasks));
    REQUIRE(tc.size() == 1);
}

TEST_CASE("Steal request") {
    MAKE_TASK_COLLECTION(tc);
    tc.steal();
    REQUIRE(comm.sent.size() == 1);
    tc.steal();
    REQUIRE(comm.sent.size() == 1);
}

TEST_CASE("Stealable tasks are LIFO") {
    MAKE_TASK_COLLECTION(tc);
    int x = 0;
    tc.add_task({[&] () { x = 1; }});
    tc.add_task({[&] () { x *= 2; }});
    tc.run_next();
    tc.run_next();
    REQUIRE(x == 1);
}

TEST_CASE("Local tasks can't be stolen") {
    MAKE_TASK_COLLECTION(tc);
    tc.add_local_task({[&] () {}});
    REQUIRE(tc.size() == 1);
    REQUIRE(tc.victimized().size() == 0);
}

TEST_CASE("Mixed tasks run LIFO") {
    MAKE_TASK_COLLECTION(tc);
    int x = 0;
    tc.add_task({[&] () { x = 1; }});
    tc.add_local_task({[&] () { x *= 2; }});
    tc.run_next();
    tc.run_next();
    REQUIRE(x == 1);
}

TEST_CASE("Send remotely assigned task") {
    MAKE_TASK_COLLECTION(tc);
    int x = 0;
    tc.add_task({1}, {[&] () { x = 1; }});
    REQUIRE(comm.sent.size() == 1);
    tc.add_local_task(std::move(comm.sent[0]));
    tc.run_next();
    REQUIRE(x == 1);
}
