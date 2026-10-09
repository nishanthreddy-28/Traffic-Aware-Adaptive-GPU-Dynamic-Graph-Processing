#include "Hornet.hpp"
#include <fstream>
#include <sstream>
#include <chrono>

namespace hornet {

std::vector<UpdateOp> DynamicUpdate::loadUpdateStream(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open update stream file: " + filepath);
    }

    std::vector<UpdateOp> stream;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string op_type;
        VertexId u, v;
        if (!(iss >> op_type >> u >> v)) continue;

        if (op_type == "INSERT") {
            EdgeWeight w = 1;
            iss >> w;
            stream.push_back({UpdateType::INSERT, u, v, w});
        } else if (op_type == "DELETE") {
            stream.push_back({UpdateType::DELETE, u, v, 1});
        }
    }

    return stream;
}

DynamicUpdateResult DynamicUpdate::executeStream(
    HornetGraph& graph, 
    const std::vector<UpdateOp>& stream
) {
    DynamicUpdateResult result;
    auto total_start = std::chrono::high_resolution_clock::now();

    for (const auto& op : stream) {
        if (op.type == UpdateType::INSERT) {
            auto ins_start = std::chrono::high_resolution_clock::now();
            bool ok = EdgeInsertion::insertEdge(graph, op.src, op.dst, op.weight);
            auto ins_end = std::chrono::high_resolution_clock::now();
            result.insertion_time_us += std::chrono::duration<double, std::micro>(ins_end - ins_start).count();
            if (ok) result.insertions_executed++;
        } else if (op.type == UpdateType::DELETE) {
            auto del_start = std::chrono::high_resolution_clock::now();
            bool ok = EdgeDeletion::deleteEdge(graph, op.src, op.dst);
            auto del_end = std::chrono::high_resolution_clock::now();
            result.deletion_time_us += std::chrono::duration<double, std::micro>(del_end - del_start).count();
            if (ok) result.deletions_executed++;
        }
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    result.total_update_time_us = std::chrono::duration<double, std::micro>(total_end - total_start).count();
    result.graph_consistent = graph.verifyConsistency();

    return result;
}

} // namespace hornet
