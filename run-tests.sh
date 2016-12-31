#!/bin/bash
OUTPUT="/tmp/a.out"
INCLUDE="-I variant/include"
WARNINGS="-Wall -Wextra -pedantic -Wpointer-arith -Wshadow -Wfloat-conversion -Wno-unused-function -Wno-unused-parameter"
COMPILE="c++ -g -std=c++14 $INCLUDE $WARNINGS -o $OUTPUT"

# Tests write errors to stderr and non-errors to stdout

$COMPILE test-traverse.cpp test-link.cpp && $OUTPUT >/dev/null
$COMPILE test-int-encoding.cpp && $OUTPUT >/dev/null
$COMPILE test-json.cpp && $OUTPUT >/dev/null
$COMPILE test-variant.cpp && $OUTPUT >/dev/null
$COMPILE test-lua.cpp -l lua && $OUTPUT >/dev/null
