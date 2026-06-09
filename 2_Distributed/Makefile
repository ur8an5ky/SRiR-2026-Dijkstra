# Kompilator UPC++ (wrapper na g++)
UPCXX := upcxx
UPCXXFLAGS := -O2 -std=c++17

# Conduit GASNet: udp w pracowni (zgodnie z README.UPC++)
CONDUIT := udp

TARGET := DijkstraUPCXX
SRC := src/DijkstraUPCXX.cpp
BUILD_DIR := build
DATA_FILE := data/matrix_25.txt

# Liczba zadań (ranks) i plik z węzłami
NP := 4
NODES := nodes
HEAP := 256M

.PHONY: all run run-local clean reset

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(SRC)
	mkdir -p $(BUILD_DIR)
	UPCXX_GASNET_CONDUIT=$(CONDUIT) $(UPCXX) $(UPCXXFLAGS) -o $@ $^

# Uruchomienie w pracowni 
run: all
	/opt/nfs/config/station204_name_list.sh 1 $(NP) > $(NODES)
	upcxx-run -shared-heap $(HEAP) -n $(NP) $$(upcxx-nodes $(NODES)) ./$(BUILD_DIR)/$(TARGET) $(DATA_FILE)

# Uruchomienie lokalne
run-local: all
	upcxx-run -localhost -shared-heap $(HEAP) -n $(NP) ./$(BUILD_DIR)/$(TARGET) $(DATA_FILE)

clean:
	rm -rf $(BUILD_DIR) $(NODES)

reset: clean