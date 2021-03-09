// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef TEST_H
#define TEST_H

#include "traverse.h"
#include <iostream>

// Helper function for the unit tests. It's verbose.

template <typename A, typename B>
void _test_equal(const A& a, const B& b, const char* file, int line, bool quiet) {
  if (a != b) {
    std::cerr << " * FAIL " << file << ':' << std::dec << line << ": (" << std::hex << a << ") != (" << std::hex << b << ")" << std::endl;
  } else if (!quiet) {
    std::cout << "   PASS " << file << ':' << std::dec << line << ": (" << std::hex << a << ")" << std::endl;
  }
}
#define TEST_EQ(a, b) _test_equal(a, b, __FILE__, __LINE__, false)
#define TEST_EQ_QUIET(a, b) _test_equal(a, b, __FILE__, __LINE__, true)


// Define some data types used in the test modules

// Basic structures, numbers
struct Point {
  int x, y;
};
TRAVERSE_STRUCT(Point, FIELD(x) FIELD(y))


// C and C++ enums, including char to test the signed/unsigned encoding
enum Color { RED, BLUE };
enum class Mood : unsigned int { HAPPY, SAD, HULK_SMASH };
enum class Signed : int { NEGATIVE = -1, ZERO, ONE };
enum class Charred : char { START, END };

// Nested structure with enums, strings, vectors
struct Polygon {
  Color color;
  Mood mood;
  Charred charred;
  std::string name;
  std::vector<Point> points;
};
TRAVERSE_STRUCT(Polygon, FIELD(color) FIELD(mood) FIELD(charred) FIELD(name) FIELD(points))



#endif
