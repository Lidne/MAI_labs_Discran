#include <iostream>
#include <vector>
#include <string>

int main() {
    std::string s;
    if (!(std::cin >> s)) {
        return 0;
    }

    const int n = static_cast<int>(s.size());
    if (n == 0) {
        std::cout << 0 << '\n';
        return 0;
    }

    std::vector<std::vector<unsigned long long>> dp(n, std::vector<unsigned long long>(n, 0));

    for (int len = 1; len <= n; ++len) {
        for (int l = 0; l + len - 1 < n; ++l) {
            int r = l + len - 1;
            if (l == r) {
                dp[l][r] = 1; // если одна буква
            } else if (s[l] == s[r]) {
                dp[l][r] = dp[l + 1][r] + dp[l][r - 1] + 1;
            } else {
                dp[l][r] = dp[l + 1][r] + dp[l][r - 1] - dp[l + 1][r - 1];
            }
        }
    }

    unsigned long long result = dp[0][n - 1];
    std::cout << result << '\n';
    return 0;
}


