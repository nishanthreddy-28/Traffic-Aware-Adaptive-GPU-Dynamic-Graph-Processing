#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <cassert>

namespace hornet {

// ============================================================================
// 1. DATA TYPES & STRUCTURES
// ============================================================================

using VertexId = uint32_t;
using EdgeWeight = int32_t;
constexpr VertexId INVALID_VERTEX = static_cast<VertexId>(-1);

/**
 * @brief Vertex metadata tracking active degree and allocated power-of-two capacity.
 */
struct Vertex {
    VertexId id{INVALID_VERTEX};
    size_t degree{0};       // Number of active outgoing edges
    size_t capacity{0};     // Capacity allocated in memory block (2^k)
    size_t block_scale{0};  // Scale k where capacity = 2^k
    
    Vertex() = default;
    explicit Vertex(VertexId vid, size_t cap = 0, size_t scale = 0)
        : id(vid), degree(0), capacity(cap), block_scale(scale) {}
};

/**
 * @brief Edge representation (source, destination, weight).
 */
struct Edge {
    VertexId src{INVALID_VERTEX};
    VertexId dst{INVALID_VERTEX};
    EdgeWeight weight{1};

    Edge() = default;
    Edge(VertexId s, VertexId d, EdgeWeight w = 1)
        : src(s), dst(d), weight(w) {}

    bool operator==(const Edge& other) const {
        return src == other.src && dst == other.dst;
    }
};

/**
 * @brief Adjacency block holding destination vertex IDs and weights.
 * Models Hornet's contiguous block-array memory allocation per vertex.
 */
struct AdjacencyBlock {
    std::vector<VertexId> dst;
    std::vector<EdgeWeight> weights;

    void resize(size_t new_cap) {
        dst.resize(new_cap, INVALID_VERTEX);
        weights.resize(new_cap, 0);
    }
};

// ============================================================================
// 2. HORNET DYNAMIC GRAPH CLASS
// ============================================================================

class HornetGraph {
public:
    explicit HornetGraph(size_t num_vertices = 0);

    size_t numVertices() const { return vertices_.size(); }
    size_t numEdges() const { return total_edges_; }
    void initVertices(size_t count);

    const Vertex& getVertex(VertexId v) const;
    size_t getDegree(VertexId v) const;
    size_t getCapacity(VertexId v) const;

    bool hasEdge(VertexId u, VertexId v) const;
    EdgeWeight getWeight(VertexId u, VertexId v) const;

    const VertexId* getNeighborArray(VertexId v) const;
    const EdgeWeight* getWeightArray(VertexId v) const;

    AdjacencyBlock& getBlock(VertexId v) { return adj_blocks_[v]; }
    const AdjacencyBlock& getBlock(VertexId v) const { return adj_blocks_[v]; }
    Vertex& getVertexRef(VertexId v) { return vertices_[v]; }

    void incrementEdgeCount() { ++total_edges_; }
    void decrementEdgeCount() { assert(total_edges_ > 0); --total_edges_; }

    void loadFromMatrixMarket(const std::string& filepath);
    bool verifyConsistency() const;

private:
    std::vector<Vertex> vertices_;
    std::vector<AdjacencyBlock> adj_blocks_;
    size_t total_edges_{0};
};

// ============================================================================
// 3. EDGE INSERTION & DELETION MODULES
// ============================================================================

class EdgeInsertion {
public:
    static bool insertEdge(HornetGraph& graph, VertexId u, VertexId v, EdgeWeight w = 1);
    static size_t insertBatch(HornetGraph& graph, const std::vector<Edge>& edges);
    static double measureInsertBatch(HornetGraph& graph, const std::vector<Edge>& edges);
};

class EdgeDeletion {
public:
    static bool deleteEdge(HornetGraph& graph, VertexId u, VertexId v);
    static size_t deleteBatch(HornetGraph& graph, const std::vector<Edge>& edges);
    static double measureDeleteBatch(HornetGraph& graph, const std::vector<Edge>& edges);
};

// ============================================================================
// 4. DYNAMIC UPDATES MODULE
// ============================================================================

enum class UpdateType { INSERT, DELETE };

struct UpdateOp {
    UpdateType type;
    VertexId src;
    VertexId dst;
    EdgeWeight weight{1};
};

struct DynamicUpdateResult {
    size_t insertions_executed{0};
    size_t deletions_executed{0};
    double insertion_time_us{0.0};
    double deletion_time_us{0.0};
    double total_update_time_us{0.0};
    bool graph_consistent{false};
};

class DynamicUpdate {
public:
    static std::vector<UpdateOp> loadUpdateStream(const std::string& filepath);
    static DynamicUpdateResult executeStream(HornetGraph& graph, const std::vector<UpdateOp>& stream);
};

// ============================================================================
// 5. GRAPH TRAVERSAL & QUERY MODULES
// ============================================================================

struct BFSResult {
    VertexId source{INVALID_VERTEX};
    std::vector<int32_t> distances;
    std::vector<VertexId> parents;
    size_t visited_count{0};
    size_t edges_traversed{0};
    double elapsed_time_us{0.0};
};

struct AdjacencyTraversalResult {
    size_t total_visited_vertices{0};
    size_t total_visited_edges{0};
    uint64_t checksum{0};
    double elapsed_time_us{0.0};
};

class GraphTraversal {
public:
    static BFSResult runBFS(const HornetGraph& graph, VertexId source);
    static AdjacencyTraversalResult runAdjacencyTraversal(const HornetGraph& graph);
};

struct SSSPResult {
    VertexId source{INVALID_VERTEX};
    std::vector<EdgeWeight> distances;
    std::vector<VertexId> predecessors;
    size_t settled_vertices{0};
    double elapsed_time_us{0.0};
};

class GraphQuery {
public:
    static SSSPResult runSSSP(const HornetGraph& graph, VertexId source);
    static EdgeWeight queryShortestPath(const HornetGraph& graph, VertexId source, VertexId target);
};

// ============================================================================
// 6. BENCHMARK SUITE
// ============================================================================

struct OperationMeasurement {
    std::string name;
    std::vector<double> run_times_us;
    double avg_time_us{0.0};
    double min_time_us{0.0};
    double max_time_us{0.0};
    double percentage{0.0};
    std::string code_region;
    std::string why_expensive;
    std::string parallel_suitability;
};

class BenchmarkSuite {
public:
    explicit BenchmarkSuite(size_t num_runs = 5);
    void runBenchmarks(const std::string& dataset_path, const std::string& updates_path);
    void printReport() const;

private:
    size_t num_runs_{5};
    size_t num_vertices_{0};
    size_t num_edges_{0};
    size_t num_insertions_{0};
    size_t num_deletions_{0};
    std::vector<OperationMeasurement> operations_;
    void calculateStats();
};

bool runCorrectnessTests();

} // namespace hornet
