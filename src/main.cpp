#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

const int MAX_NODES = 10000000;

struct Node {
    int l, r;
    int sum;
} tree[MAX_NODES];

int node_cnt = 0;

int build(int tl, int tr) {
    int id = ++node_cnt;
    tree[id].sum = 0;
    if (tl == tr) {
        tree[id].l = tree[id].r = 0;
        return id;
    }
    int tm = (tl + tr) / 2;
    tree[id].l = build(tl, tm);
    tree[id].r = build(tm + 1, tr);
    return id;
}

int update(int prev, int tl, int tr, int pos, int val) {
    int id = ++node_cnt;
    tree[id] = tree[prev];
    if (tl == tr) {
        tree[id].sum += val;
        return id;
    }
    int tm = (tl + tr) / 2;
    if (pos <= tm)
        tree[id].l = update(tree[prev].l, tl, tm, pos, val);
    else
        tree[id].r = update(tree[prev].r, tm + 1, tr, pos, val);

    tree[id].sum = tree[tree[id].l].sum + tree[tree[id].r].sum;
    return id;
}

int query(int u, int tl, int tr, int q_idx) {
    if (q_idx < tl) return 0;
    if (tr <= q_idx) return tree[u].sum;

    int tm = (tl + tr) / 2;
    return query(tree[u].l, tl, tm, q_idx) +
           query(tree[u].r, tm + 1, tr, q_idx);
}

struct Segment {
    int l, r, h;
};

bool cmp(const Segment& a, const Segment& b) {
    return a.h > b.h;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n, m;
    if (!(cin >> n >> m)) return 0;

    vector<Segment> segs(n);
    vector<int> coords;
    coords.reserve(2 * n + m);

    for (int i = 0; i < n; ++i) {
        cin >> segs[i].l >> segs[i].r >> segs[i].h;
        coords.push_back(segs[i].l);
        coords.push_back(segs[i].r + 1);
    }

    struct Query {
        int x, y;
    };
    vector<Query> queries(m);
    for (int i = 0; i < m; ++i) {
        cin >> queries[i].x >> queries[i].y;
        coords.push_back(queries[i].x);
    }

    sort(coords.begin(), coords.end());
    coords.erase(unique(coords.begin(), coords.end()), coords.end());

    auto get_pos = [&](int val) {
        return lower_bound(coords.begin(), coords.end(), val) - coords.begin();
    };

    int sz = coords.size();
    sort(segs.begin(), segs.end(), cmp);

    vector<pair<int, int>> history;

    int current_root = build(0, sz - 1);

    for (const auto& s : segs) {
        int l_idx = get_pos(s.l);
        int r_idx = get_pos(s.r + 1);

        current_root = update(current_root, 0, sz - 1, l_idx, 1);
        current_root = update(current_root, 0, sz - 1, r_idx, -1);

        history.push_back({s.h, current_root});
    }

    for (int i = 0; i < m; ++i) {
        int x = queries[i].x;
        int y = queries[i].y;

        auto it = lower_bound(history.begin(), history.end(), y, [](const pair<int, int>& p, int val) {
            return p.first > val;
        });

        int ans = 0;
        if (it != history.begin()) {
            int ver_root = prev(it)->second;
            int x_pos = get_pos(x);
            ans = query(ver_root, 0, sz - 1, x_pos);
        }

        cout << ans << "\n";
    }

    return 0;
}
