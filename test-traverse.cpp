// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "traverse.h"
#include <iostream>
#include "test.h"


int main() {
  traverse::CoutWriter writer;
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
    for (size_t i = 0; i < msg.size(); i++) {
      out << int(msg[i]) << ' ';
    }
    TEST_EQ(out.str(), "1 2 9 85 70 79 34 49 57 52 50 34 3 6 10 8 12 10 14 ");

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
    std::cout << "__ Integer size grew __" << std::endl;
    int16_t narrow = -1563;
    int64_t wide = 0xdeadbeefdeadbeef;
    traverse::BinarySerialize s;
    visit(s, narrow);
    traverse::BinaryDeserialize u(s.out.str());
    visit(u, wide);
    TEST_EQ(narrow, wide);
    TEST_EQ(u.Errors(), "");
  }
    
  {
    std::cout << "__ Integer size shrunk __" << std::endl;
    uint64_t wide = 17291729;
    uint16_t narrow = 0xdead;
    traverse::BinarySerialize s;
    visit(s, wide);
    traverse::BinaryDeserialize u(s.out.str());
    visit(u, narrow);
    TEST_EQ(narrow, wide & 0xffff);
    TEST_EQ(u.Errors(), "");
  }
    
  {
    std::cout << "__ Corrupt deserialize __ " << std::endl;
    std::string msg = serialize.out.str();
    for (size_t i = 0; i < msg.size(); i++) {
      msg[i] = 0x7f;
    }
    traverse::BinaryDeserialize reader(msg);
    Polygon polygon2;
    visit(reader, polygon2);
    TEST_EQ(reader.Errors().substr(0, 5), "Error");
  }

  {
    std::cout << "__ Serialized message too short __ " << std::endl;
    std::string msg = serialize.out.str();
    msg.erase(msg.size()/2);
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
}
