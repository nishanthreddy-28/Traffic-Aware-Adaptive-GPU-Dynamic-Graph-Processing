# Traffic-Aware Adaptive GPU Dynamic Graph Processing
## Sequential CPU Baseline Implementation & Hotspot Identification of the Hornet Base Paper

---

## Executive Summary
This project provides the **sequential CPU baseline implementation and hotspot identification** for the base paper:
> **"Hornet: An Efficient Data Structure for Dynamic Sparse Graphs and Matrices on GPUs"** (Busato et al., IEEE HPEC 2018).

In strict adherence to project instructions:
* **Sequential CPU Only**: Written in standard C++17 with CPU-only execution.
* **NO Parallelism**: OpenMP, CUDA, pthreads, std::thread, MPI, and GPU kernels have **NOT** been implemented.
* **Goal Achieved**: Hornet paper → Sequential CPU implementation → Correctness → Performance measurement → Hotspot identification.
* **Modular Multi-File Architecture**: Cleanly split into separate `.cpp` files matching the required modules, sharing a single unified header [`src/Hornet.hpp`](file:///d:/HPC/hornet_seq/src/Hornet.hpp).

---

## 1. Project Directory Structure

```text
d:/HPC/hornet_seq
 │
 ├── src/                        # Modular C++ source code & single shared header
 │    ├── Hornet.hpp             # Single shared header (Vertex, Edge, HornetGraph declarations)
 │    ├── HornetGraph.cpp        # Module 1 & 2: Graph representation & Matrix Market parser
 │    ├── EdgeInsertion.cpp      # Module 3: Sequential edge insertion (geometric growth)
 │    ├── EdgeDeletion.cpp       # Module 4: Sequential edge deletion (O(1) compaction)
 │    ├── DynamicUpdate.cpp      # Module 5: Dynamic update stream execution
 │    ├── GraphTraversal.cpp     # Module 6: Sequential BFS & adjacency traversal
 │    ├── GraphQuery.cpp         # Module 7: Dijkstra SSSP & shortest path queries
 │    ├── Benchmark.cpp          # Module 8: Multi-run timing & hotspot analysis
 │    └── main.cpp               # Module 9: Correctness test suite & CLI driver
 │
 ├── data/                       # Preprocessed benchmark datasets & updates
 │    ├── dblp_metadata.json     # Graph properties and subset extraction metadata
 │    ├── dblp_subset.edges      # Directed edge list (V=100, E=794)
 │    ├── dblp_subset.mtx        # Matrix Market format (V=100, E=397 symmetric)
 │    ├── dblp_updates.txt       # Deterministic 130-operation update stream
 │    └── raw/                   # Original base paper dataset archive
 │         └── dblp-2010.mtx     # Base paper dataset (326,186 nodes from SuiteSparse)
 │
 ├── scripts/                    # Preprocessing & extraction scripts
 │    ├── find_seed.py           # Seed node search script
 │    └── preprocess_dblp.py     # Deterministic subset extraction script
 │
 ├── Makefile                    # Single-command build file: 'make'
 ├── CMakeLists.txt              # Standard CMake build file
 ├── README.md                   # Full evaluation report and documentation
 └── hornet_seq.exe              # Compiled binary executable
```

---

## 2. Dataset & Deterministic Extraction

* **Base Paper Dataset**: `LAW/dblp-2010` (Scientific collaboration co-authorship network) from the SuiteSparse Matrix Collection, evaluated directly in **Table II** of the Hornet paper (*Busato et al., 2018*).
* **Original Size**: $|V| = 326,186$ vertices, $|E| = 807,700$ undirected non-zeros ($1,615,400$ directed edges).
* **Extraction Method**: Deterministic Breadth-First Search (BFS) expansion from high-centrality author seed `112425` (original degree: 126).
* **Extracted Subset**: Exactly **$V = 100$ vertices** and **$E = 397$ undirected edges** ($794$ directed arcs), meeting the target range ($V \approx 100, E \approx 300\text{--}500$).
* **Dynamic Stream**: 130 deterministic updates (50 insertions, 50 deletions, 15 insertions, 15 deletions) in [`data/dblp_updates.txt`](file:///d:/HPC/hornet_seq/data/dblp_updates.txt).

---

## 3. How to Compile

Using `make`:
```bash
make
```

Or using direct `g++`:
```bash
g++ -std=c++17 -O3 -Wall -Wextra -Isrc \
    src/HornetGraph.cpp \
    src/EdgeInsertion.cpp \
    src/EdgeDeletion.cpp \
    src/DynamicUpdate.cpp \
    src/GraphTraversal.cpp \
    src/GraphQuery.cpp \
    src/Benchmark.cpp \
    src/main.cpp \
    -o hornet_seq.exe
```

---

## 4. How to Run

```bash
./hornet_seq.exe --runs 5
```

CLI options:
* `--runs <N>`: Set the number of benchmark iterations (default: 5).
* `--dataset <path>`: Specify path to Matrix Market file (default: `data/dblp_subset.mtx`).
* `--updates <path>`: Specify path to update stream file (default: `data/dblp_updates.txt`).

---

## 5. Correctness Validation

Before profiling, 6 automated test suites execute in `main()`:
1. `[PASS]` **Graph Construction**: Verifies vertices, edges, and bidirectionality on a diamond topology.
2. `[PASS]` **Edge Insertion**: Verifies new edge creation and duplicate edge weight updates.
3. `[PASS]` **Edge Deletion**: Verifies $O(1)$ swap-with-last compaction and degree decrement.
4. `[PASS]` **Graph Traversal (BFS)**: Verifies exact level distances and reachable node counts.
5. `[PASS]` **Shortest Path (SSSP)**: Verifies Dijkstra shortest distances against analytical truth.
6. `[PASS]` **Dynamic Updates**: Verifies total edge counts and degree consistency across the update stream.

---

## 6. Performance Measurements & Hotspot Analysis (5-Run Average)

```text
================================================
HORNET BASE PAPER
SEQUENTIAL CPU IMPLEMENTATION
================================================

Dataset (LAW/dblp-2010 Subset)
Vertices : 100
Edges    : 794 (directed)

------------------------------------------------
PERFORMANCE SUMMARY (5-Run Average)
------------------------------------------------
Operation                   Time(ms)    Time(us)  Percentage
------------------------------------------------------------
Graph Construction            0.5692      569.18      89.50%
Edge Insertion                0.0049        4.86       0.76%
Edge Deletion                 0.0043        4.28       0.67%
Dynamic Updates               0.0330       33.02       5.19%
Graph Traversal (BFS)         0.0058        5.78       0.91%
Shortest Path (SSSP)          0.0157       15.70       2.47%
Adjacency Traversal           0.0031        3.14       0.49%
------------------------------------------------------------

================================================
HOTSPOT ANALYSIS (Dynamic Profile)
================================================

Top Hotspots:
  1. Graph Construction (89.50% of total runtime)
  2. Dynamic Updates (5.19% of total runtime)
  3. Shortest Path (SSSP) (2.47% of total runtime)
```

---

## 7. Code-Level Hotspot Details & Future Parallelization Candidates

| Hotspot Rank | Operation | Runtime | Loop Section | Why It Is Expensive | Future Parallelization Candidate (Next Stage) |
| :---: | :--- | :---: | :--- | :--- | :--- |
| **#1** | **Graph Construction** | $569.18\ \mu\text{s}$ ($89.50\%$) | `while (getline)` + `EdgeInsertion::insertEdge` | Sequential file stream parsing and repeated geometric block allocations ($2^1 \to 2^2 \dots$). | Bulk-load edge list using parallel radix sort and parallel prefix sums on GPU. |
| **#2** | **Dynamic Updates** | $33.02\ \mu\text{s}$ ($5.19\%$) | `DynamicUpdate::executeStream` + `deleteEdge` loop | Serial dispatch of interleaved updates; linear scan for edge index in block array. | Batch-parallel updates on GPU with warp-level ballot lookup and lock-free memory compaction. |
| **#3** | **Shortest Path (SSSP)** | $15.70\ \mu\text{s}$ ($2.47\%$) | `while (!pq.empty())` + neighbor relaxation loop | Priority queue extract-min serialization + neighbor relaxation pointer traversal. | Frontier-parallel $\Delta$-stepping or Bellman-Ford relaxation on GPU thread blocks. |
