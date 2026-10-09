CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra -Isrc

SRCS = src/HornetGraph.cpp \
       src/EdgeInsertion.cpp \
       src/EdgeDeletion.cpp \
       src/DynamicUpdate.cpp \
       src/GraphTraversal.cpp \
       src/GraphQuery.cpp \
       src/Benchmark.cpp \
       src/main.cpp

TARGET = hornet_seq.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET) *.o

run: $(TARGET)
	./$(TARGET) --runs 5
