#include "Hornet.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cassert>

using namespace hornet;

bool hornet::runCorrectnessTests() {
    std::cout << "================================================\n";
    std::cout << "SECTION 8: CORRECTNESS VALIDATION SUITE\n";
    std::cout << "================================================\n";

    // 1. Manually verifiable diamond test graph
    HornetGraph test_g(4);
    assert(EdgeInsertion::insertEdge(test_g, 0, 1, 2));
    assert(EdgeInsertion::insertEdge(test_g, 0, 2, 5));
    assert(EdgeInsertion::insertEdge(test_g, 1, 3, 4));
    assert(EdgeInsertion::insertEdge(test_g, 2, 3, 1));

    if (test_g.numVertices() != 4 || test_g.numEdges() != 4) return false;
    if (!test_g.hasEdge(0, 1) || !test_g.hasEdge(0, 2) || !test_g.hasEdge(1, 3) || !test_g.hasEdge(2, 3)) return false;
    std::cout << "[PASS] Graph Construction & Adjacency Relationships verified.\n";

    // 2. Edge Insertion Verification
    assert(EdgeInsertion::insertEdge(test_g, 1, 2, 3));
    if (!test_g.hasEdge(1, 2) || test_g.getDegree(1) != 2 || test_g.numEdges() != 5) return false;
    assert(!EdgeInsertion::insertEdge(test_g, 1, 2, 7)); // duplicate handling
    if (test_g.getWeight(1, 2) != 7) return false;
    std::cout << "[PASS] Edge Insertion & Duplicate Protection verified.\n";

    // 3. Edge Deletion Verification
    assert(EdgeDeletion::deleteEdge(test_g, 1, 2));
    if (test_g.hasEdge(1, 2) || test_g.getDegree(1) != 1 || test_g.numEdges() != 4) return false;
    assert(!EdgeDeletion::deleteEdge(test_g, 1, 2)); // deleting non-existent edge
    std::cout << "[PASS] Edge Deletion & Compaction verified.\n";

    // 4. Traversal Verification (BFS)
    auto bfs_res = GraphTraversal::runBFS(test_g, 0);
    if (bfs_res.visited_count != 4 || bfs_res.distances[3] != 2) return false;
    std::cout << "[PASS] Graph Traversal (BFS) reachable vertices & levels verified.\n";

    // 5. Shortest Path Verification (SSSP / Dijkstra)
    auto sssp_res = GraphQuery::runSSSP(test_g, 0);
    if (sssp_res.distances[0] != 0 || sssp_res.distances[1] != 2 || 
        sssp_res.distances[2] != 5 || sssp_res.distances[3] != 6) return false;
    std::cout << "[PASS] Shortest Path (SSSP/Dijkstra) distance calculation verified.\n";

    // 6. Dynamic Updates Verification
    std::vector<UpdateOp> updates = {
        {UpdateType::INSERT, 0, 3, 10},
        {UpdateType::DELETE, 0, 1, 1},
        {UpdateType::INSERT, 1, 0, 2},
        {UpdateType::DELETE, 0, 3, 1}
    };
    auto dyn_res = DynamicUpdate::executeStream(test_g, updates);
    if (!dyn_res.graph_consistent) return false;
    std::cout << "[PASS] Dynamic Update stream & Consistency verification passed.\n";

    std::cout << "All Correctness Tests PASSED successfully!\n\n";
    return true;
}

int main(int argc, char* argv[]) {
    std::string dataset_path = "data/dblp_subset.mtx";
    std::string updates_path = "data/dblp_updates.txt";
    size_t num_runs = 5;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dataset" && i + 1 < argc) {
            dataset_path = argv[++i];
        } else if (arg == "--updates" && i + 1 < argc) {
            updates_path = argv[++i];
        } else if (arg == "--runs" && i + 1 < argc) {
            num_runs = std::stoul(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "  --dataset <path>  Matrix Market dataset file (default: data/dblp_subset.mtx)\n"
                      << "  --updates <path>  Dynamic update stream file (default: data/dblp_updates.txt)\n"
                      << "  --runs <N>        Number of benchmark runs (default: 5)\n";
            return 0;
        }
    }

    // Step 1: Run Correctness Suite
    if (!hornet::runCorrectnessTests()) {
        std::cerr << "CRITICAL ERROR: Correctness verification failed. Aborting benchmark.\n";
        return 1;
    }

    // Step 2: Run Performance Benchmark Suite
    BenchmarkSuite suite(num_runs);
    suite.runBenchmarks(dataset_path, updates_path);

    // Step 3: Print Expected Output & Hotspot Analysis
    suite.printReport();

    return 0;
}
