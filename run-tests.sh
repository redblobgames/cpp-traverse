#!/bin/bash
OUTPUT="/tmp/a.out"
COMPILE="c++ -g -std=c++14 -o $OUTPUT"

$COMPILE test-traverse.cpp test-link.cpp && $OUTPUT
$COMPILE test-int-encoding.cpp && $OUTPUT
$COMPILE test-json.cpp && $OUTPUT

