#include <bits/stdc++.h>
using namespace std;

struct Entry {
    array<uint8_t, 6> key;
    char value[65];
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<Entry> entries;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;
        auto tab = line.find('\t');

        string key_s;
        key_s.reserve(6);
        for (char c : line.substr(0, tab)) {
            if (isalnum(static_cast<unsigned char>(c)))
                key_s.push_back(c);
        }
        if (key_s.size() != 6) continue;

        Entry e;

        auto val = line.substr(tab + 1);
        size_t len = min(val.size(), size_t(64));
        memcpy(e.value, val.data(), len);
        e.value[len] = '\0';

        for (int i = 0; i < 6; ++i) {
            char c = key_s[i];
            e.key[i] = isalpha(static_cast<unsigned char>(c)) ? c - 'A' : c - '0';
        }
        entries.push_back(e);
    }

    size_t n = entries.size();
    vector<Entry> buffer(n);
    array<int, 26> cnt;
    array<int, 26> pos_arr;

    for (int d = 5; d >= 0; --d) {
        int range = (d == 0 || d >= 4 ? 26 : 10);
        cnt.fill(0);
        for (auto &e : entries) cnt[e.key[d]]++;
        pos_arr[0] = 0;
        for (int i = 1; i < range; ++i)
            pos_arr[i] = pos_arr[i - 1] + cnt[i - 1];

        for (auto &e : entries) {
            int k = e.key[d];
            buffer[pos_arr[k]++] = e;
        }
        entries.swap(buffer);
    }

    for (auto &e : entries) {
        cout << char('A' + e.key[0]) << ' ';
        cout << int(e.key[1]) << int(e.key[2]) << int(e.key[3]) << ' ';
        cout << char('A' + e.key[4]) << char('A' + e.key[5]);
        cout << '\t' << e.value << '\n';
    }
    return 0;
}
