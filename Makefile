.RECIPEPREFIX = >

PROJECT := usync
INCLUDE_DIR := include
BUILD_DIR := build
FLAGS := -std=c++20 -I$(INCLUDE_DIR)
DEBUG_FLAGS := $(FLAGS) -g -O0
RELEASE_FLAGS := $(FLAGS) -O2

TEST_EXE := $(BUILD_DIR)/$(PROJECT)-test
TEST_SOURCES = test/test.cpp $(shell find test -name "*.hpp")


all: dir $(TEST_EXE)


dir:
> mkdir -p $(BUILD_DIR)
.PHONY: dir


$(TEST_EXE): $(TEST_SOURCES)
> $(CXX) $(TEST_SOURCES) -o $(TEST_EXE) -$(DEBUG_FLAGS)


test: $(TEST_EXE)
> $(TEST_EXE)
.PHONY: test


clean:
> rm -f $(BUILD_DIR)/*
.PHONY: clean
