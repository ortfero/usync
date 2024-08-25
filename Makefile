.RECIPEPREFIX = >

PROJECT := usync
INCLUDE_DIR := include
BUILD_DIR := build
FLAGS := -std=c++20 -I$(INCLUDE_DIR)
DEBUG_FLAGS := $(FLAGS) -g -O0
RELEASE_FLAGS := $(FLAGS) -O2

HEADERS := $(wildcard $(INCLUDE_DIR)/$(PROJECT)/*.hpp)

TEST_EXE := $(BUILD_DIR)/$(PROJECT)-test
TEST_HEADERS := $(wildcard test/*.hpp)
TEST_SOURCES := test/test.cpp
TEST_DEPS := $(TEST_SOURCES) $(TEST_HEADERS) $(HEADERS)


all: dir $(TEST_EXE)


dir:
> mkdir -p $(BUILD_DIR)
.PHONY: dir


$(TEST_EXE): $(TEST_DEPS)
> $(CXX) $(TEST_SOURCES) -o $(TEST_EXE) -$(DEBUG_FLAGS)


test: $(TEST_EXE)
> $(TEST_EXE)
.PHONY: test


clean:
> rm -f $(BUILD_DIR)/*
.PHONY: clean

