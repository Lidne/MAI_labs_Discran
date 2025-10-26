#include <cmath>
#include <iostream>
#include <vector>

struct Coin {
    unsigned int value;
    unsigned int count;
};

void printShi(std::vector<Coin> arr) {
    for (auto& coin : arr) {
        std::cout << coin.count << std::endl;
    }
}

int main() {
    unsigned int N, p, M;
    std::cin >> N >> p >> M;
    std::vector<Coin> arr(N);
    for (size_t idx = 0; idx < N; ++idx) {
        arr[idx].value = pow(p, idx);
    }
    // printShi(arr);

    size_t i = N - 1;
    while (M != 0) {
        arr[i].count = M / arr[i].value;
        M -= arr[i].count * arr[i].value;
        --i;
    }
    printShi(arr);
    return 0;
}