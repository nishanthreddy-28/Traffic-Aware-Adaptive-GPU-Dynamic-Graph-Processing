#include "Hornet.hpp"
#include <chrono>

namespace hornet {

bool EdgeDeletion::deleteEdge(HornetGraph& graph, VertexId u, VertexId v) {
    if (u >= graph.numVertices() || v >= graph.numVertices()) {
        return false;
    }

    auto& vert = graph.getVertexRef(u);
    auto& block = graph.getBlock(u);

    // 1. Sequential search through u's active adjacency block
    size_t found_idx = vert.degree;
    for (size_t i = 0; i < vert.degree; ++i) {
        if (block.dst[i] == v) {
            found_idx = i;
            break;
        }
    }

    if (found_idx >= vert.degree) {
        return false; // Edge does not exist
    }

    // 2. Hornet O(1) in-place swap-with-last compaction
    size_t last_idx = vert.degree - 1;
    if (found_idx != last_idx) {
        block.dst[found_idx] = block.dst[last_idx];
        block.weights[found_idx] = block.weights[last_idx];
    }
    block.dst[last_idx] = INVALID_VERTEX;
    block.weights[last_idx] = 0;
    vert.degree--;
    graph.decrementEdgeCount();

    // 3. Hornet block shrinking when memory utilization is low (< 25% and cap > 4)
    if (vert.capacity > 4 && vert.degree <= vert.capacity / 4) {
        size_t new_cap = vert.capacity / 2;
        vert.capacity = new_cap;
        vert.block_scale = (vert.block_scale > 0) ? vert.block_scale - 1 : 0;
        block.resize(new_cap);
    }

    return true;
}

size_t EdgeDeletion::deleteBatch(HornetGraph& graph, const std::vector<Edge>& edges) {
    size_t count = 0;
    for (const auto& e : edges) {
        if (deleteEdge(graph, e.src, e.dst)) {
            count++;
        }
    }
    return count;
}

double EdgeDeletion::measureDeleteBatch(HornetGraph& graph, const std::vector<Edge>& edges) {
    auto start = std::chrono::high_resolution_clock::now();
    deleteBatch(graph, edges);
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
}

} // namespace hornet
