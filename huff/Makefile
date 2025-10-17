# Ensure GCC/ld use a writable temp on Windows
export TMP    := C:/Users/anton/AppData/Local/Temp
export TEMP   := $(TMP)
export TMPDIR := $(TMP)

# --- Toolchain & Flags ---
CXX      := g++
CXXFLAGS := -O3 -std=c++20 -pthread -Wall -Wextra -pipe -MMD -MP -Iinclude

# --- Files & Folders ---
BIN  := huff.exe
SRCS := $(wildcard src/*.cpp)
OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# --- Default Target ---
all: $(BIN)

# --- Link ---
$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# --- Compile to build/ ---
build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Ensure build/ exists ---
build:
	mkdir -p $@

# --- Utilities ---
.PHONY: clean run
clean:
	rm -f $(BIN) build/*.o build/*.d

run: $(BIN)
	./$(BIN)

# --- Auto-include generated dependency files ---
-include $(DEPS)
