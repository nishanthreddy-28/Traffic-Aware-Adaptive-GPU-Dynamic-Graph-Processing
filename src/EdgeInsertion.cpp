#include "Hornet.hpp"
#include <chrono>
#include <algorithm>

namespace hornet {

bool EdgeInsertion::insertEdge(HornetGraph& graph, VertexId u, VertexId v, EdgeWeight w) {
    if (u >= graph.numVertices() || v >= graph.numVertices()) {
        size_t required = std::max(u, v) + 1;
        graph.initVertices(required);
    }

    auto& vert = graph.getVertexRef(u);
    auto& block = graph.getBlock(u);

    // 1. Sequential search through active neighbors to prevent duplicate edges
    for (size_t i = 0; i < vert.degree; ++i) {
        if (block.dst[i] == v) {
            block.weights[i] = w; // update weight if exists
            return false;
        }
    }

    // 2. Hornet geometric block reallocation when degree reaches capacity (2^k -> 2^{k+1})
    if (vert.degree >= vert.capacity) {
        size_t new_cap = (vert.capacity == 0) ? 2 : vert.capacity * 2;
        vert.block_scale = (vert.capacity == 0) ? 1 : vert.block_scale + 1;
        vert.capacity = new_cap;
        block.resize(new_cap);
    }

    // 3. Append edge to the contiguous block
    block.dst[vert.degree] = v;
    block.weights[vert.degree] = w;
    vert.degree++;
    graph.incrementEdgeCount();

    return true;
}

size_t EdgeInsertion::insertBatch(HornetGraph& graph, const std::vector<Edge>& edges) {
    size_t count = 0;
    for (const auto& e : edges) {
        if (insertEdge(graph, e.src, e.dst, e.weight)) {
            count++;
        }
    }
    return count;
}

double EdgeInsertion::measureInsertBatch(HornetGraph& graph, const std::vector<Edge>& edges) {
    auto start = std::chrono::high_resolution_clock::now();
    insertBatch(graph, edges);
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
}

} // namespace hornet
