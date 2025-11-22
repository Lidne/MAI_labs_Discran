#include <iostream>
#include <vector>
#include <limits>

struct Edge {
    int from;
    int to;
    long long weight;
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n, m, start, finish;
    std::cin >> n >> m >> start >> finish;

    std::vector<Edge> edges;
    edges.reserve(m);
    for (int i = 0; i < m; ++i) {
        int u, v;
        long long w;
        std::cin >> u >> v >> w;
        edges.push_back({u - 1, v - 1, w});
    }

    const long long INF = 10000000000LL;
    std::vector<long long> dist(n, INF);
    dist[start - 1] = 0;

    for (int i = 0; i < n - 1; ++i) {
        bool updated = false;
        for (const auto& edge : edges) {
            if (dist[edge.from] == INF) {
                continue;
            }
            const long long candidate = dist[edge.from] + edge.weight;
            if (candidate < dist[edge.to]) {
                dist[edge.to] = candidate;
                updated = true;
            }
        }
        if (!updated) {
            break;
        }
    }

    const long long answer = dist[finish - 1];
    if (answer == INF) {
        std::cout << "No solution\n";
    } else {
        std::cout << answer << '\n';
    }

    return 0;
}