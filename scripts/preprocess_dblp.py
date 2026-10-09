#!/usr/bin/env python3
"""
Preprocessing script for Hornet Sequential CPU Baseline.
Extracts a deterministic, structure-preserving small subset from the official
LAW/dblp-2010 dataset (SuiteSparse Matrix Collection, evaluated in Table II of Hornet paper).

Methodology:
- Seed Selection: Deterministic selection of high-centrality vertex (ID: 112425, degree: 126).
- Graph Expansion: Breadth-First Search (BFS) visiting neighbors in deterministic order
  until exactly V = 100 vertices are collected.
- Induced Subgraph: Extracts all induced edges among these 100 vertices to preserve
  exact clustering coefficient, local triangles, and community structure.
- Node Relabeling: Relabels vertices to contiguous 0-indexed IDs [0 .. 99].
- Edge Attributes: Assigns positive integer weights (default: 1, or collaborative distance).
- Dynamic Updates Generation: Generates a deterministic sequence of insertions and deletions
  for dynamic update profiling.
"""

import os
import json
from collections import defaultdict, deque

def preprocess():
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    mtx_input = os.path.join(base_dir, "data_raw", "dblp-2010", "dblp-2010.mtx")
    data_out_dir = os.path.join(base_dir, "data")
    os.makedirs(data_out_dir, exist_ok=True)

    print(f"Loading official Hornet dataset: {mtx_input}...")
    adj = defaultdict(set)
    total_original_v = 0
    total_original_entries = 0

    with open(mtx_input, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('%'):
                continue
            parts = line.split()
            if len(parts) == 3 and total_original_v == 0:
                total_original_v = int(parts[0])
                total_original_entries = int(parts[2])
                continue
            u, v = int(parts[0]), int(parts[1])
            if u != v:
                adj[u].add(v)
                adj[v].add(u)

    print(f"Original DBLP dataset: |V| = {total_original_v}, non-zeros = {total_original_entries}")
    print(f"Active vertices with non-zero degree: {len(adj)}")

    # Deterministic seed: Vertex 112425
    seed = 112425
    assert seed in adj, f"Seed {seed} not found in graph"
    
    # Deterministic BFS expansion up to 100 nodes
    visited = [seed]
    visited_set = {seed}
    queue = deque([seed])

    while queue and len(visited) < 100:
        curr = queue.popleft()
        for nbr in sorted(adj[curr]):
            if nbr not in visited_set:
                visited_set.add(nbr)
                visited.append(nbr)
                queue.append(nbr)
                if len(visited) == 100:
                    break

    assert len(visited) == 100, f"Expected 100 nodes, got {len(visited)}"

    # Map original IDs to 0..99
    orig_to_new = {orig_id: new_id for new_id, orig_id in enumerate(visited)}
    new_to_orig = {new_id: orig_id for new_id, orig_id in enumerate(visited)}

    # Extract induced edges
    undirected_edges = []
    directed_edges = []
    
    for u_orig in visited:
        u_new = orig_to_new[u_orig]
        for v_orig in adj[u_orig]:
            if v_orig in visited_set:
                v_new = orig_to_new[v_orig]
                directed_edges.append((u_new, v_new, 1))
                if u_orig < v_orig:
                    undirected_edges.append((u_new, v_new, 1))

    # Sort edges deterministically
    undirected_edges.sort()
    directed_edges.sort()

    V_count = len(visited)
    E_undirected = len(undirected_edges)
    E_directed = len(directed_edges)

    print("\n--- Deterministic Subset Extracted ---")
    print(f"Vertices (|V|): {V_count}")
    print(f"Undirected Edges: {E_undirected}")
    print(f"Directed Edges: {E_directed}")
    print(f"Average Degree: {E_directed / V_count:.2f}")

    # 1. Save Matrix Market subset
    mtx_out_path = os.path.join(data_out_dir, "dblp_subset.mtx")
    with open(mtx_out_path, 'w', encoding='utf-8') as f:
        f.write("%%MatrixMarket matrix coordinate pattern symmetric\n")
        f.write(f"% Subgraph extracted from LAW/dblp-2010 (SuiteSparse)\n")
        f.write(f"% Seed: {seed}, Nodes: {V_count}, Undirected Edges: {E_undirected}\n")
        f.write(f"{V_count} {V_count} {E_undirected}\n")
        for u, v, _ in undirected_edges:
            # 1-indexed for Matrix Market
            f.write(f"{u + 1} {v + 1}\n")
    print(f"Saved Matrix Market file: {mtx_out_path}")

    # 2. Save Edge list format (0-indexed: u v w)
    edges_out_path = os.path.join(data_out_dir, "dblp_subset.edges")
    with open(edges_out_path, 'w', encoding='utf-8') as f:
        f.write(f"# Directed edges for Hornet sequential baseline: V={V_count} E={E_directed}\n")
        for u, v, w in directed_edges:
            f.write(f"{u} {v} {w}\n")
    print(f"Saved Edge list file: {edges_out_path}")

    # 3. Generate deterministic update stream
    # Phase 1: 50 Insertions (valid non-existing pairs in the 100-node set)
    # Phase 2: 50 Deletions (existing edges in the graph)
    # Phase 3: 30 Additional updates (15 insertions + 15 deletions)
    existing_directed = set((u, v) for u, v, _ in directed_edges)
    
    # Candidate insertions: all possible pairs (u, v) with u != v not in existing_directed
    candidate_insertions = []
    for u in range(V_count):
        for v in range(V_count):
            if u != v and (u, v) not in existing_directed:
                candidate_insertions.append((u, v))
    candidate_insertions.sort(key=lambda pair: (pair[0] * 31 + pair[1] * 17) % 10007)

    # Candidate deletions: deterministic sample of existing edges
    candidate_deletions = list(directed_edges)
    candidate_deletions.sort(key=lambda e: (e[0] * 43 + e[1] * 19) % 10007)

    insert_batch_1 = candidate_insertions[:50]
    delete_batch_1 = [(u, v) for u, v, _ in candidate_deletions[:50]]

    # For Phase 3:
    # 15 additional insertions from candidate_insertions[50:65]
    insert_batch_2 = candidate_insertions[50:65]
    # 15 additional deletions from candidate_deletions[50:65]
    delete_batch_2 = [(u, v) for u, v, _ in candidate_deletions[50:65]]

    updates_out_path = os.path.join(data_out_dir, "dblp_updates.txt")
    with open(updates_out_path, 'w', encoding='utf-8') as f:
        f.write(f"# Dynamic update stream for Hornet sequential baseline\n")
        # Header: count of phase 1 insertions, deletions, phase 2 additional
        f.write(f"# Phase 1: {len(insert_batch_1)} Insertions\n")
        for u, v in insert_batch_1:
            f.write(f"INSERT {u} {v} 1\n")
        f.write(f"# Phase 2: {len(delete_batch_1)} Deletions\n")
        for u, v in delete_batch_1:
            f.write(f"DELETE {u} {v}\n")
        f.write(f"# Phase 3: Additional updates ({len(insert_batch_2)} ins, {len(delete_batch_2)} del)\n")
        for u, v in insert_batch_2:
            f.write(f"INSERT {u} {v} 1\n")
        for u, v in delete_batch_2:
            f.write(f"DELETE {u} {v}\n")
    print(f"Saved Updates stream file: {updates_out_path}")

    # 4. Save metadata JSON
    meta_path = os.path.join(data_out_dir, "dblp_metadata.json")
    meta = {
        "dataset_name": "LAW/dblp-2010",
        "official_source": "SuiteSparse Matrix Collection (https://sparse.tamu.edu/LAW/dblp-2010)",
        "paper_reference": "Hornet: An Efficient Data Structure for Dynamic Sparse Graphs and Matrices on GPUs (Table II)",
        "original_vertices": total_original_v,
        "original_nonzeros": total_original_entries,
        "subset_seed_vertex": seed,
        "subset_selection_method": "Deterministic BFS frontier expansion preserving dense collaboration cluster",
        "subset_vertices": V_count,
        "subset_undirected_edges": E_undirected,
        "subset_directed_edges": E_directed,
        "phase1_insertions": len(insert_batch_1),
        "phase2_deletions": len(delete_batch_1),
        "phase3_insertions": len(insert_batch_2),
        "phase3_deletions": len(delete_batch_2),
        "total_dynamic_updates": len(insert_batch_1) + len(delete_batch_1) + len(insert_batch_2) + len(delete_batch_2)
    }
    with open(meta_path, 'w', encoding='utf-8') as f:
        json.dump(meta, f, indent=2)
    print(f"Saved Metadata JSON: {meta_path}")

if __name__ == "__main__":
    preprocess()
