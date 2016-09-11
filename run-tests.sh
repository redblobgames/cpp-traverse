#!/bin/bash
OUTPUT="/tmp/a.out"
COMPILE="c++ -g -std=c++14 -Wall -o $OUTPUT"

# TODO: -Wextra -pedantic -Wpointer-arith -Wshadow -Wfloat-conversion -Wno-unused-function -Wno-unused-parameter -Wno-sign-compare

# Tests write errors to stderr and non-errors to stdout

$COMPILE test-traverse.cpp test-link.cpp && $OUTPUT >/dev/null
$COMPILE test-int-encoding.cpp && $OUTPUT >/dev/null
$COMPILE test-json.cpp && $OUTPUT >/dev/null
$COMPILE test-lua.cpp -l lua && $OUTPUT >/dev/null
