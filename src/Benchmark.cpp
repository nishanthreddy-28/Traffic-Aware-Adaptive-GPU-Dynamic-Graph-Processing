#include "Hornet.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <chrono>

namespace hornet {

BenchmarkSuite::BenchmarkSuite(size_t num_runs) : num_runs_(num_runs) {}

void BenchmarkSuite::runBenchmarks(const std::string& dataset_path, const std::string& updates_path) {
    operations_ = {
        {"Graph Construction", {}, 0, 0, 0, 0,
         "HornetGraph::loadFromMatrixMarket() -> EdgeInsertion::insertEdge()",
         "I/O parsing, file reading, and sequential block allocations.",
         "Parallel graph ingestion / parallel CSR-to-Hornet conversion."},

        {"Edge Insertion", {}, 0, 0, 0, 0,
         "EdgeInsertion::insertEdge() -> duplicate check loop + block realloc",
         "Linear scan for existing edges O(deg) + dynamic block doubling O(cap).",
         "Parallel batch insertion / GPU warp-level duplicate scan and allocation."},

        {"Edge Deletion", {}, 0, 0, 0, 0,
         "EdgeDeletion::deleteEdge() -> linear search loop in block.dst",
         "Sequential scan of active block elements to find target edge index.",
         "Parallel batch deletion / GPU block-level compaction."},

        {"Dynamic Updates", {}, 0, 0, 0, 0,
         "DynamicUpdate::executeStream() -> interleaved insertion/deletion loop",
         "Sequential dispatch of interleaved updates and alternating memory modifications.",
         "GPU batch update streaming kernel with lock-free memory compaction."},

        {"Graph Traversal (BFS)", {}, 0, 0, 0, 0,
         "GraphTraversal::runBFS() -> frontier expansion neighbor traversal loop",
         "Sequential queue pop and inner loop iterating over vertex neighbor arrays.",
         "Frontier-parallel BFS (warp-per-vertex or block-per-vertex on GPU)."},

        {"Shortest Path (SSSP)", {}, 0, 0, 0, 0,
         "GraphQuery::runSSSP() -> priority queue loop + edge relaxation loop",
         "Priority queue extract-min serialization + neighbor relaxation traversal.",
         "Delta-stepping or Bellman-Ford parallel relaxation on GPU."},

        {"Adjacency Traversal", {}, 0, 0, 0, 0,
         "GraphTraversal::runAdjacencyTraversal() -> nested (u, v) loops",
         "Full graph iteration traversing all contiguous neighbor arrays in memory.",
         "Vertex-parallel or edge-parallel grid dispatch across GPU threads."}
    };

    auto update_stream = DynamicUpdate::loadUpdateStream(updates_path);
    std::vector<Edge> insert_batch;
    std::vector<Edge> delete_batch;
    for (const auto& op : update_stream) {
        if (op.type == UpdateType::INSERT && insert_batch.size() < 50) {
            insert_batch.emplace_back(op.src, op.dst, op.weight);
        } else if (op.type == UpdateType::DELETE && delete_batch.size() < 50) {
            delete_batch.emplace_back(op.src, op.dst, 1);
        }
    }
    num_insertions_ = insert_batch.size();
    num_deletions_ = delete_batch.size();

    std::cout << "================================================\n";
    std::cout << "STARTING REPRODUCIBLE SEQUENTIAL BENCHMARK\n";
    std::cout << "Runs: " << num_runs_ << " iterations\n";
    std::cout << "================================================\n\n";

    for (size_t run = 1; run <= num_runs_; ++run) {
        std::cout << ">>> Executing Run " << run << " of " << num_runs_ << " ...\n";

        HornetGraph graph;
        auto t_start = std::chrono::high_resolution_clock::now();
        graph.loadFromMatrixMarket(dataset_path);
        auto t_end = std::chrono::high_resolution_clock::now();
        operations_[0].run_times_us.push_back(std::chrono::duration<double, std::micro>(t_end - t_start).count());

        if (run == 1) {
            num_vertices_ = graph.numVertices();
            num_edges_ = graph.numEdges();
        }

        HornetGraph g_ins = graph;
        operations_[1].run_times_us.push_back(EdgeInsertion::measureInsertBatch(g_ins, insert_batch));

        HornetGraph g_del = graph;
        operations_[2].run_times_us.push_back(EdgeDeletion::measureDeleteBatch(g_del, delete_batch));

        HornetGraph g_dyn = graph;
        auto dyn_res = DynamicUpdate::executeStream(g_dyn, update_stream);
        operations_[3].run_times_us.push_back(dyn_res.total_update_time_us);

        auto bfs_res = GraphTraversal::runBFS(graph, 0);
        operations_[4].run_times_us.push_back(bfs_res.elapsed_time_us);

        auto sssp_res = GraphQuery::runSSSP(graph, 0);
        operations_[5].run_times_us.push_back(sssp_res.elapsed_time_us);

        auto adj_res = GraphTraversal::runAdjacencyTraversal(graph);
        operations_[6].run_times_us.push_back(adj_res.elapsed_time_us);
    }

    calculateStats();
}

void BenchmarkSuite::calculateStats() {
    double total_avg = 0.0;
    for (auto& op : operations_) {
        if (op.run_times_us.empty()) continue;
        double sum = std::accumulate(op.run_times_us.begin(), op.run_times_us.end(), 0.0);
        op.avg_time_us = sum / op.run_times_us.size();
        op.min_time_us = *std::min_element(op.run_times_us.begin(), op.run_times_us.end());
        op.max_time_us = *std::max_element(op.run_times_us.begin(), op.run_times_us.end());
        total_avg += op.avg_time_us;
    }
    for (auto& op : operations_) {
        if (total_avg > 0.0) op.percentage = (op.avg_time_us / total_avg) * 100.0;
    }
}

void BenchmarkSuite::printReport() const {
    std::cout << "\n================================================\n";
    std::cout << "HORNET BASE PAPER\n";
    std::cout << "SEQUENTIAL CPU IMPLEMENTATION\n";
    std::cout << "================================================\n\n";

    std::cout << "Dataset (LAW/dblp-2010 Subset)\n";
    std::cout << "Vertices : " << num_vertices_ << "\n";
    std::cout << "Edges    : " << num_edges_ << " (directed)\n\n";

    std::cout << "------------------------------------------------\n";
    std::cout << "GRAPH CONSTRUCTION\n";
    std::cout << "------------------------------------------------\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Time : " << (operations_[0].avg_time_us / 1000.0) << " ms (" 
              << operations_[0].avg_time_us << " us)\n\n";

    std::cout << "------------------------------------------------\n";
    std::cout << "DYNAMIC UPDATES\n";
    std::cout << "------------------------------------------------\n";
    std::cout << "Insertions : " << num_insertions_ << "\n";
    std::cout << "Deletions  : " << num_deletions_ << "\n";
    std::cout << "Time       : " << (operations_[3].avg_time_us / 1000.0) << " ms (" 
              << operations_[3].avg_time_us << " us)\n\n";

    std::cout << "------------------------------------------------\n";
    std::cout << "GRAPH PROCESSING\n";
    std::cout << "------------------------------------------------\n";
    std::cout << "BFS Time            : " << (operations_[4].avg_time_us / 1000.0) << " ms (" 
              << operations_[4].avg_time_us << " us)\n";
    std::cout << "Shortest Path Time  : " << (operations_[5].avg_time_us / 1000.0) << " ms (" 
              << operations_[5].avg_time_us << " us)\n";
    std::cout << "Adjacency Traversal : " << (operations_[6].avg_time_us / 1000.0) << " ms (" 
              << operations_[6].avg_time_us << " us)\n\n";

    std::cout << "------------------------------------------------\n";
    std::cout << "PERFORMANCE SUMMARY (" << num_runs_ << "-Run Average)\n";
    std::cout << "------------------------------------------------\n";
    std::cout << std::left << std::setw(24) << "Operation"
              << std::right << std::setw(12) << "Time(ms)"
              << std::setw(12) << "Time(us)"
              << std::setw(12) << "Percentage" << "\n";
    std::cout << "------------------------------------------------------------\n";

    for (const auto& op : operations_) {
        std::cout << std::left << std::setw(24) << op.name
                  << std::right << std::fixed << std::setprecision(4)
                  << std::setw(12) << (op.avg_time_us / 1000.0)
                  << std::fixed << std::setprecision(2)
                  << std::setw(12) << op.avg_time_us
                  << std::setw(11) << op.percentage << "%\n";
    }
    std::cout << "------------------------------------------------------------\n";

    // Hotspot Analysis
    std::vector<OperationMeasurement> sorted_ops = operations_;
    std::sort(sorted_ops.begin(), sorted_ops.end(), [](const auto& a, const auto& b) {
        return a.avg_time_us > b.avg_time_us;
    });

    std::cout << "\n================================================\n";
    std::cout << "HOTSPOT ANALYSIS (Dynamic Profile)\n";
    std::cout << "================================================\n\n";

    std::cout << "Operation                 Time(ms)       Time(us)       Percentage\n";
    std::cout << "------------------------------------------------------------------\n";
    for (const auto& op : sorted_ops) {
        std::cout << std::left << std::setw(26) << op.name
                  << std::right << std::fixed << std::setprecision(4)
                  << std::setw(10) << (op.avg_time_us / 1000.0) << " ms"
                  << std::fixed << std::setprecision(2)
                  << std::setw(12) << op.avg_time_us << " us"
                  << std::setw(10) << op.percentage << "%\n";
    }

    std::cout << "\nTop Hotspots:\n\n";
    for (size_t i = 0; i < std::min<size_t>(3, sorted_ops.size()); ++i) {
        std::cout << "  " << (i + 1) << ". " << sorted_ops[i].name 
                  << " (" << sorted_ops[i].percentage << "% of total runtime)\n";
    }

    std::cout << "\n================================================\n";
    std::cout << "CODE-LEVEL STATIC & DYNAMIC HOTSPOT BREAKDOWN\n";
    std::cout << "================================================\n\n";

    for (size_t i = 0; i < std::min<size_t>(3, sorted_ops.size()); ++i) {
        const auto& op = sorted_ops[i];
        std::cout << "Hotspot #" << (i + 1) << ": " << op.name << "\n";
        std::cout << "  Runtime (Avg)     : " << (op.avg_time_us / 1000.0) << " ms (" << op.avg_time_us << " us)\n";
        std::cout << "  Percentage        : " << op.percentage << "%\n";
        std::cout << "  Code Region       : " << op.code_region << "\n";
        std::cout << "  Why Expensive     : " << op.why_expensive << "\n";
        std::cout << "  Future Parallelism: " << op.parallel_suitability << "\n";
        std::cout << "------------------------------------------------\n";
    }

    std::cout << "\n================================================\n";
    std::cout << "SEQUENTIAL BASELINE COMPLETE\n";
    std::cout << "================================================\n";
}

} // namespace hornet
