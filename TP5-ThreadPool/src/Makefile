# Compiler settings - Can change to clang++ if preferred
CXX = g++
CXXFLAGS = -std=c++14 -Wall -pthread -g
# cambio c++11 to c++14 (para poder usar std::make_unique)

# Build targets
TARGET = threadpool
SRC = thread-pool.cc Semaphore.cc main.cc

# Link the target with object files
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

custom:
	$(CXX) $(CXXFLAGS) -o $(TARGET) thread-pool.cc Semaphore.cc test_custom.cc

# Clean up build artifacts
clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean

# # Source files
# COMMON_SRC = thread-pool.cc Semaphore.cc

# # Targets
# all: main tptest tpcustomtest

# main: $(COMMON_SRC) main.cc
# 	$(CXX) $(CXXFLAGS) -o $@ $^

# tptest: $(COMMON_SRC) tptest.cc
# 	$(CXX) $(CXXFLAGS) -o $@ $^

# tpcustomtest: $(COMMON_SRC) tpcustomtest.cc
# 	$(CXX) $(CXXFLAGS) -o $@ $^

# clean:
# 	rm -f main tptest tpcustomtest
