#include "Hornet.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace hornet {

HornetGraph::HornetGraph(size_t num_vertices) {
    if (num_vertices > 0) initVertices(num_vertices);
}

void HornetGraph::initVertices(size_t count) {
    vertices_.clear();
    adj_blocks_.clear();
    total_edges_ = 0;

    vertices_.reserve(count);
    adj_blocks_.resize(count);

    for (size_t i = 0; i < count; ++i) {
        // Initial block allocation: capacity = 2 = 2^1
        vertices_.emplace_back(static_cast<VertexId>(i), 2, 1);
        adj_blocks_[i].resize(2);
    }
}

const Vertex& HornetGraph::getVertex(VertexId v) const {
    return vertices_.at(v);
}

size_t HornetGraph::getDegree(VertexId v) const {
    return (v < vertices_.size()) ? vertices_[v].degree : 0;
}

size_t HornetGraph::getCapacity(VertexId v) const {
    return (v < vertices_.size()) ? vertices_[v].capacity : 0;
}

bool HornetGraph::hasEdge(VertexId u, VertexId v) const {
    if (u >= vertices_.size() || v >= vertices_.size()) return false;
    const auto& vert = vertices_[u];
    const auto& block = adj_blocks_[u];
    for (size_t i = 0; i < vert.degree; ++i) {
        if (block.dst[i] == v) return true;
    }
    return false;
}

EdgeWeight HornetGraph::getWeight(VertexId u, VertexId v) const {
    if (u >= vertices_.size() || v >= vertices_.size()) return 0;
    const auto& vert = vertices_[u];
    const auto& block = adj_blocks_[u];
    for (size_t i = 0; i < vert.degree; ++i) {
        if (block.dst[i] == v) return block.weights[i];
    }
    return 0;
}

const VertexId* HornetGraph::getNeighborArray(VertexId v) const {
    if (v >= adj_blocks_.size()) return nullptr;
    return adj_blocks_[v].dst.data();
}

const EdgeWeight* HornetGraph::getWeightArray(VertexId v) const {
    if (v >= adj_blocks_.size()) return nullptr;
    return adj_blocks_[v].weights.data();
}

void HornetGraph::loadFromMatrixMarket(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open Matrix Market file: " + filepath);
    }

    std::string line;
    bool is_symmetric = false;
    bool found_header = false;

    // 1. Parse banner
    if (std::getline(file, line)) {
        if (line.find("%%MatrixMarket") == 0) {
            std::string lower_line = line;
            std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);
            if (lower_line.find("symmetric") != std::string::npos) {
                is_symmetric = true;
            }
        }
    }

    size_t rows = 0, cols = 0, nonzeros = 0;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '%') continue;
        std::istringstream iss(line);
        if (iss >> rows >> cols >> nonzeros) {
            found_header = true;
            break;
        }
    }

    if (!found_header) {
        throw std::runtime_error("Invalid Matrix Market file: missing size header");
    }

    initVertices(rows);

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '%') continue;
        std::istringstream iss(line);
        VertexId u_1, v_1;
        if (!(iss >> u_1 >> v_1)) continue;
        if (u_1 == 0 || v_1 == 0) continue;

        VertexId u = u_1 - 1;
        VertexId v = v_1 - 1;
        EdgeWeight w = 1;
        iss >> w;

        if (u != v) {
            EdgeInsertion::insertEdge(*this, u, v, w);
            if (is_symmetric) {
                EdgeInsertion::insertEdge(*this, v, u, w);
            }
        }
    }
}

bool HornetGraph::verifyConsistency() const {
    size_t edge_sum = 0;
    for (size_t i = 0; i < vertices_.size(); ++i) {
        const auto& vert = vertices_[i];
        const auto& block = adj_blocks_[i];

        if (vert.degree > vert.capacity) return false;
        if (block.dst.size() != vert.capacity) return false;

        for (size_t j = 0; j < vert.degree; ++j) {
            if (block.dst[j] >= vertices_.size()) return false;
        }
        edge_sum += vert.degree;
    }
    return (edge_sum == total_edges_);
}

} // namespace hornet
