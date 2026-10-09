@echo off
echo Compiling Hornet Sequential CPU Baseline...
g++ -std=c++17 -O3 -Wall -Wextra -Isrc src\HornetGraph.cpp src\EdgeInsertion.cpp src\EdgeDeletion.cpp src\DynamicUpdate.cpp src\GraphTraversal.cpp src\GraphQuery.cpp src\Benchmark.cpp src\main.cpp -o hornet_seq.exe
if %ERRORLEVEL% EQU 0 (
    echo Build successful: hornet_seq.exe created!
) else (
    echo Build failed!
)
