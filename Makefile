# --- CONFIGURATION ---
CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -I.
LDFLAGS  := -pthread

# --- REPERTOIRES ---
SRC_DIR  := src
OBJ_DIR  := obj

# --- FICHIERS ---
SRCS     := main.cpp $(wildcard $(SRC_DIR)/*.cpp)
OBJS     := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS))
TARGET   := simulation


# Mode par défaut
all: release

# Mode Débogage (Non optimisé)
debug: CXXFLAGS += -O0 -g
debug: $(TARGET)

# Mode Release (Optimisé pour la remise)
release: CXXFLAGS += -O2
release: $(TARGET)

# --- REGLES DE COMPILATION ---

# Liaison finale
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Compilation des fichiers .cpp en .o
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -rf $(OBJ_DIR) $(TARGET) test/rapports

.PHONY: all clean debug release