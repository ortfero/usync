project := "usync"
include_dir := "include"
test-file := project + "-test"
flags := "-std=c++20 -I" + include_dir
debug_flags := flags + " -g -O0"
release_flags := flags + " -O3"

alias b := build

default: test

build-test:
    mkdir -p build
    c++ test/test.cpp -o build/{{test-file}} {{debug_flags}}

build: build-test

test: build-test
    build/{{test-file}}

clean:
    rm -rf ./build

