#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

std::vector<int> buildSA(const std::string &text) {
    int n = text.length();
    std::vector<int> sa(n);
    for (int i = 0; i < n; i++) {
        sa[i] = i;
    }
    auto suffixComparator = [&text](int a, int b) {
     return std::lexicographical_compare(text.begin() + a, text.end(), text.begin() + b, text.end()); };
    std::sort(sa.begin(), sa.end(), suffixComparator);
    
    return sa;
}

// void printSA(const std::vector<int>& sa) {
//     std::cout << "-------SA-------\n";
//     int n = sa.size();
//     for (int i = 0; i < n; i++) {
//         std::cout << sa[i] << "\n";
//     }
//     std::cout << "-------end-------" << std::endl;
// }

std::vector<int> searchSA(const std::string &text, const std::string &pattern, std::vector<int> &sa) {
    int patternLength = pattern.size();
    int arrSize = text.size();
    int l = 0, r = arrSize - 1;
    
    while (l <= r) {
        int center = l + (r - l) / 2;
        if (text.substr(sa[center], patternLength) < pattern) {
            l = center + 1;
        } 
        else {
            r = center - 1;
        }
    }
    
    int firstOccurrence = l;
    r = arrSize - 1;

    while (l <= r) {
        int center = l + (r - l) / 2;
        if (text.substr(sa[center], patternLength) <= pattern) {
            l = center + 1;
        } 
        else {
            r = center - 1;
        }
    }

    int lastOccurrence = r;

    std::vector<int> resultPositions;
    for (int idx = firstOccurrence; idx <= lastOccurrence; idx++) {
        resultPositions.push_back(sa[idx]);
    }

    return resultPositions;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string text;
    if (!std::getline(std::cin, text))
        return 0;

    std::vector<int> sa = buildSA(text);
    // printSA(sa);

    std::string pattern;
    int count = 1;
    while (std::getline(std::cin, pattern))
    {
        std::vector<int> foundPositions = searchSA(text, pattern, sa);
        std::sort(foundPositions.begin(), foundPositions.end());
        if(foundPositions.empty()) {
            count++;
            continue;
        }
        int positionsSize = foundPositions.size();
        std::cout << count << ": ";
        for (int i = 0; i < positionsSize; i++) {
            if(i == positionsSize - 1) {
                std::cout << foundPositions[i] + 1;
            }
            else {
                std::cout << foundPositions[i] + 1 << ", ";
            }
        }
        std::cout << std::endl;
        count++;
    }
    return 0;
}
