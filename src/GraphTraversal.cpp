#include "Hornet.hpp"
#include <queue>
#include <chrono>

namespace hornet {

BFSResult GraphTraversal::runBFS(const HornetGraph& graph, VertexId source) {
    BFSResult result;
    result.source = source;
    size_t V = graph.numVertices();
    result.distances.assign(V, -1);
    result.parents.assign(V, INVALID_VERTEX);

    if (source >= V) {
        return result;
    }

    auto start = std::chrono::high_resolution_clock::now();

    std::queue<VertexId> q;
    result.distances[source] = 0;
    result.parents[source] = source;
    q.push(source);
    result.visited_count = 1;

    while (!q.empty()) {
        VertexId curr = q.front();
        q.pop();

        size_t deg = graph.getDegree(curr);
        const VertexId* nbrs = graph.getNeighborArray(curr);

        for (size_t i = 0; i < deg; ++i) {
            VertexId next = nbrs[i];
            result.edges_traversed++;
            if (result.distances[next] == -1) {
                result.distances[next] = result.distances[curr] + 1;
                result.parents[next] = curr;
                result.visited_count++;
                q.push(next);
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.elapsed_time_us = std::chrono::duration<double, std::micro>(end - start).count();

    return result;
}

AdjacencyTraversalResult GraphTraversal::runAdjacencyTraversal(const HornetGraph& graph) {
    AdjacencyTraversalResult result;
    size_t V = graph.numVertices();

    auto start = std::chrono::high_resolution_clock::now();

    for (VertexId u = 0; u < V; ++u) {
        size_t deg = graph.getDegree(u);
        const VertexId* nbrs = graph.getNeighborArray(u);
        const EdgeWeight* w = graph.getWeightArray(u);
        result.total_visited_vertices++;

        for (size_t i = 0; i < deg; ++i) {
            VertexId v = nbrs[i];
            result.checksum += (static_cast<uint64_t>(u) * 31 + static_cast<uint64_t>(v) + static_cast<uint64_t>(w[i]));
            result.total_visited_edges++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.elapsed_time_us = std::chrono::duration<double, std::micro>(end - start).count();

    return result;
}

} // namespace hornet
