// This file is linked in just to make sure that the headers can be
// included from more than one cpp file. The code isn't actually called.

#include "traverse.h"
#include "traverse-json.h"
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
  picojson::value json;
  traverse::JsonWriter writer3{json};

  visit(writer1, p);
  visit(writer2, p);
  visit(writer3, p);

  traverse::BinaryDeserialize reader2{writer2.out.str()};
  traverse::JsonReader reader3{json, std::cerr};
  visit(reader2, q);
  visit(reader3, q);
}
