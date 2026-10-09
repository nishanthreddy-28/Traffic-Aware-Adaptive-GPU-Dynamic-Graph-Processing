#include "Hornet.hpp"
#include <queue>
#include <chrono>
#include <limits>

namespace hornet {

SSSPResult GraphQuery::runSSSP(const HornetGraph& graph, VertexId source) {
    SSSPResult result;
    result.source = source;
    size_t V = graph.numVertices();
    const EdgeWeight INF = std::numeric_limits<EdgeWeight>::max();
    result.distances.assign(V, INF);
    result.predecessors.assign(V, INVALID_VERTEX);

    if (source >= V) {
        return result;
    }

    auto start = std::chrono::high_resolution_clock::now();

    using PQElement = std::pair<EdgeWeight, VertexId>;
    std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;

    result.distances[source] = 0;
    result.predecessors[source] = source;
    pq.push({0, source});

    while (!pq.empty()) {
        auto [curr_dist, u] = pq.top();
        pq.pop();

        if (curr_dist > result.distances[u]) {
            continue;
        }

        result.settled_vertices++;
        size_t deg = graph.getDegree(u);
        const VertexId* nbrs = graph.getNeighborArray(u);
        const EdgeWeight* weights = graph.getWeightArray(u);

        for (size_t i = 0; i < deg; ++i) {
            VertexId v = nbrs[i];
            EdgeWeight edge_w = weights[i];

            if (result.distances[u] + edge_w < result.distances[v]) {
                result.distances[v] = result.distances[u] + edge_w;
                result.predecessors[v] = u;
                pq.push({result.distances[v], v});
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.elapsed_time_us = std::chrono::duration<double, std::micro>(end - start).count();

    return result;
}

EdgeWeight GraphQuery::queryShortestPath(const HornetGraph& graph, VertexId source, VertexId target) {
    auto res = runSSSP(graph, source);
    if (target < res.distances.size()) {
        return res.distances[target];
    }
    return std::numeric_limits<EdgeWeight>::max();
}

} // namespace hornet
