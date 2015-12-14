#!/bin/bash

c++ -g -std=c++14 test-traverse.cpp test-link.cpp && ./a.out
c++ -g -std=c++14 test-varint.cpp && ./a.out

