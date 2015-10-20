// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "traverse.h"
#include <iostream>

struct Point {
  int x, y;
};

struct LineSegment {
  Point a, b;
};

struct Polygon {
  std::vector<Point> points;
};
  
namespace traverse {
  template <typename Visitor>
  void handle(Visitor& visitor, Point& value) {
    visit_struct(visitor)
      ("x", value.x)
      ("y", value.y);
  }

  template <typename Visitor>
  void handle(Visitor& visitor, LineSegment& value) {
    visit_struct(visitor)
      ("a", value.a)
      ("b", value.b);
  }

  template <typename Visitor>
  void handle(Visitor& visitor, Polygon& value) {
    visit_struct(visitor)
      ("points", value.points);
  }
}

int main() {
  traverse::CoutWriter writer;
  Point p = {3, 5};
  LineSegment s = {{1, 7}, {13, 19}};
  Polygon polygon = {{{3, 5}, {4, 6}, {5, 7}}};
  
  std::cout << "original data: " << std::endl;
  visit(writer, "polygon", polygon);
  std::cout << std::endl;

  std::cout << "serialized to bytes: " << std::endl;
  traverse::BinarySerialize writer2;
  visit(writer2, "polygon", polygon);
  std::string msg = writer2.out.str();
  for (int i = 0; i < msg.size(); i++) {
    std::cout << int(msg[i]) << ' ';
  }
  std::cout << std::endl;

  std::cout << "read back from bytes: " << std::endl;
  traverse::BinaryDeserialize reader(msg);
  Polygon polygon2;
  visit(reader, "dummy", polygon2);
  visit(writer, "polygon", polygon2);
  std::cout << std::endl;

  std::cout << "corrupting data: " << std::endl;
  msg = writer2.out.str();
  msg[3] = 0x7f;
  traverse::BinaryDeserialize reader2(msg);
  Polygon polygon3;
  visit(reader2, "dummy", polygon3);
  visit(writer, "polygon", polygon3);
  std::cout << std::endl;
}
