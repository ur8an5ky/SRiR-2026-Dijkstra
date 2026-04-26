CXX := mpic++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude

TARGET := DijkstraMPI
SRC := src/DijkstraMPI.cpp
BUILD_DIR := build
DATA_FILE := data/matrix3.txt

NP := 16
NODES := nodes

.PHONY: all run clean reset

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

run: all
	/opt/nfs/config/station204_name_list.sh 1 $(NP) > $(NODES)
	mpiexec -f $(NODES) -n $(NP) ./$(BUILD_DIR)/$(TARGET) $(DATA_FILE)

clean:
	rm -rf $(BUILD_DIR) $(NODES)

reset: clean