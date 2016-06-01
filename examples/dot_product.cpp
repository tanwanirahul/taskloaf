#include "taskloaf.hpp"
#include "taskloaf/timing.hpp"
#include "patterns.hpp"

#include <random>
#include <iostream>

using namespace taskloaf;

double random(double a, double b) {
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(a, b);
    return dis(gen);
}

std::vector<double> random_list(size_t N, double a, double b) {
    std::vector<double> es(N);
    for (size_t i = 0; i < N; ++i) {
        es[i] = random(a, b);
    }
    return es;
}

int main() {
    int n = 10000000;
    for (int n_workers = 1; n_workers <= 6; n_workers++) {
        TIC;
        auto ctx = launch_local(n_workers);
        std::vector<Future<double>> chunks;
        int n_per_block = n / n_workers;
        for (int i = 0; i < n_workers; i++) {
            chunks.push_back(async(i, [=] () {
                double out = 0;               
                for (int i = 0; i < n_per_block; i++) {
                    out += random(0, 1) * random(0, 1);
                }
                return out;
            }));
        }
        reduce(chunks, std::plus<double>()).wait();
        TOC("dot product " + std::to_string(n_workers));
    }
}
