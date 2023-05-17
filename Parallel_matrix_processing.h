#pragma once
#include <algorithm>
#include <vector>
void compute(std::vector<std::vector<int>>& matrix, std::vector<int> tasks) {
    const int n = matrix.size();
    for (int k = 0; k < tasks.size(); k++) {
        int min = matrix[0][tasks[k]];
        int cord = 0;
        for (int i = 0; i < n; i++) {
            if (matrix[i][tasks[k]] < min) {
                min = matrix[i][tasks[k]];
                cord = i;
            }
        }
        std::swap(matrix[cord][tasks[k]], matrix[n - 1 - tasks[k]][tasks[k]]);
    }
    //cout << "!!!";
}
template <typename T>
void foo(std::vector<std::vector<T>>& matrix, int amount_of_threads) {
    const int n = matrix.size();
    const int k = amount_of_threads;

    const int tasks_per_thread = n / k;
    int remainder = n % k;

    std::vector<std::thread> threads;

    int start = 0;
    for (int i = 0; i < k; ++i) {
        if (start > n) break;
        std::vector<int> tasks;
        if (remainder > 0) {
            tasks.push_back(start);
            --remainder;
            ++start;
        }

        for (int j = 0; j < tasks_per_thread; ++j) {
            tasks.push_back(start);
            ++start;
        }
        //for (auto x : tasks)
            //cout << x << " ";
        //cout << endl;
        threads.emplace_back(compute, std::ref(matrix), tasks);
    }

    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
}
