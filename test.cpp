// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "traverse.h"
#include <iostream>

template <typename A, typename B>
void _test_equal(const A& a, const B& b, const char* file, int line) {
  if (a != b) {
    std::cerr << " * FAIL " << file << ':' << line << ": (" << a << ") != (" << b << ")" << std::endl;
  } else {
    std::cerr << "   PASS " << file << ':' << line << ": (" << a << ")" << std::endl;
  }
}
#define TEST_EQ(a, b) _test_equal(a, b, __FILE__, __LINE__)


#define TEST_JSON 1
#if TEST_JSON
#include "traverse-json.h"
#endif

// Basic test of structs
struct Point {
  int x, y;
};
TRAVERSE_STRUCT(Point, FIELD(x) FIELD(y))

// Testing serialization of private fields (declare friend function)
struct LineSegment {
  LineSegment(Point a_, Point b_): a(a_), b(b_) {}
private:
  Point a, b;
  TRAVERSE_IS_FRIEND(LineSegment)
};
TRAVERSE_STRUCT(LineSegment, FIELD(a) FIELD(b))

// Testing C and C++ enums
enum Color { RED, BLUE };
enum class Mood { HAPPY, SAD, HULK_SMASH };

// Testing complex structure with enums, strings, vectors
struct Polygon {
  Color color;
  Mood mood;
  std::string name;
  std::vector<Point> points;
};
TRAVERSE_STRUCT(Polygon, FIELD(color) FIELD(mood) FIELD(name) FIELD(points))

// This just tests that everything compiles; it's not a unit test that
// makes sure things are correct.

int main() {
  traverse::CoutWriter writer;
  const Point p = {3, 5};
  const LineSegment s{{1, 7}, {13, 19}};
  const Polygon polygon = {BLUE, Mood::HULK_SMASH, "UFO\"1942\"", {{3, 5}, {4, 6}, {5, 7}}};

  std::cout << "__ Basic CoutWriter __" << std::endl;
  {
    std::stringstream out;
    out << polygon;
    TEST_EQ(out.str(), "Polygon{color:1, mood:2, name:\"UFO\\\"1942\\\"\", points:[Point{x:3, y:5}, Point{x:4, y:6}, Point{x:5, y:7}]}");
  }

  traverse::BinarySerialize serialize;
  visit(serialize, polygon);

  {
    std::cout << "__ Serialize to bytes __ " << std::endl;
    std::string msg = serialize.out.str();
    std::stringstream out;
    for (int i = 0; i < msg.size(); i++) {
      out << int(msg[i]) << ' ';
    }
    TEST_EQ(out.str(), "1 0 0 0 2 0 0 0 9 0 0 0 85 70 79 34 49 57 52 50 34 3 0 0 0 3 0 0 0 5 0 0 0 4 0 0 0 6 0 0 0 5 0 0 0 7 0 0 0 ");

    std::cout << "__ Deserialize from bytes __ " << std::endl;
    std::stringstream out1, out2;
    traverse::BinaryDeserialize reader(msg);
    Polygon polygon2;
    visit(reader, polygon2);
    out1 << polygon;
    out2 << polygon2;
    TEST_EQ(out1.str(), out2.str());
    TEST_EQ(reader.Errors(), "");
  }

  {
    std::cout << "__ Corrupt deserialize __ " << std::endl;
    std::string msg = serialize.out.str();
    msg[11] = 0x7f;
    traverse::BinaryDeserialize reader(msg);
    Polygon polygon2;
    visit(reader, polygon2);
    TEST_EQ(reader.Errors().substr(0, 5), "Error");
  }

  {
    std::cout << "__ Serialized message too short __ " << std::endl;
    std::string msg = serialize.out.str();
    msg.erase(30);
    traverse::BinaryDeserialize reader(msg);
    Polygon polygon2;
    visit(reader, polygon2);
    TEST_EQ(reader.Errors().substr(0, 5), "Error");
  }

  {
    std::cout << "__ Serialized message too long __ " << std::endl;
    std::string msg = serialize.out.str();
    msg += "12345";
    traverse::BinaryDeserialize reader(msg);
    Polygon polygon2;
    visit(reader, polygon2);
    TEST_EQ(reader.Errors().substr(0, 5), "Error");
  }
  
#if TEST_JSON
  {
    std::cout << "__ Serialize to JSON __ " << std::endl;
    picojson::value json1;
    traverse::JsonWriter jsonwriter{json1};
    visit(jsonwriter, polygon);
    TEST_EQ(json1.serialize(), "{\"color\":1,\"mood\":2,\"name\":\"UFO\\\"1942\\\"\",\"points\":[{\"x\":3,\"y\":5},{\"x\":4,\"y\":6},{\"x\":5,\"y\":7}]}");
  }

  picojson::value json2;
  {
    std::cout << "__ Deserialize from JSON to PicoJSON __ " << std::endl;
    auto err = picojson::parse(json2, "{\"points\":[{\"UNUSED\":0,\"x\":3,\"y\":5},{\"y\":6,\"x\":4},{\"y\":7},{\"x\":\"WRONGTYPE\"}]}");
    TEST_EQ(err, "");
  }

  {
    std::cout << "__ Deserialize JSON to Polygon __ " << std::endl;
    std::stringstream errors;
    traverse::JsonReader jsonreader{json2, errors};
    Polygon polygon2;
    visit(jsonreader, polygon2);
    
    std::stringstream out;
    out << polygon2;
    TEST_EQ(errors.str().substr(0, 7), "Warning");
  }
#endif
}
