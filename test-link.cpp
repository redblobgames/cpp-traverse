// This file is linked in just to make sure that the headers can be
// included from more than one cpp file. The code isn't actually called.

#include "traverse.h"
#include <iostream>

struct Point {
  int x, y;
};
TRAVERSE_STRUCT(Point, FIELD(x) FIELD(y))

void test_linkage() {
  const Point p = {3, 5};
  Point q;
  
  traverse::CoutWriter writer1;
  traverse::BinarySerialize writer2;

  visit(writer1, p);
  visit(writer2, p);

  traverse::BinaryDeserialize reader2{writer2.out.str()};
  visit(reader2, q);
}
