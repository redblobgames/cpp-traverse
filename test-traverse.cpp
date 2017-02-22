// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "traverse.h"
#include <iostream>
#include "test.h"


template<typename T>
std::string to_bytes(const T& obj) {
  std::stringbuf buf;
  traverse::BinarySerialize serialize(buf);
  visit(serialize, obj);
  std::string msg = buf.str();
  std::stringstream out;
  for (unsigned char c : msg) {
    out << int(c) << ' ';
  }
  return out.str();
}


void test_int() {
  TEST_EQ(to_bytes('@'), "128 1 ");
  TEST_EQ(to_bytes(int(0)), "0 ");
  TEST_EQ(to_bytes(int(-1)), "1 ");
  TEST_EQ(to_bytes(int(1)), "2 ");
  TEST_EQ(to_bytes(int(1024)), "128 16 ");
  TEST_EQ(to_bytes(long(1)), "2 ");
  TEST_EQ(to_bytes((unsigned int)(1)), "1 ");
}

  
void test_enum() {
  TEST_EQ(to_bytes(Mood::HULK_SMASH), "2 ");
  TEST_EQ(to_bytes(Signed::NEGATIVE), "1 ");
  TEST_EQ(to_bytes(Signed::ONE), "2 ");
}

  
int main() {
  test_int();
  test_enum();
    
  traverse::CoutWriter writer;
  const Polygon polygon = {BLUE, Mood::HULK_SMASH, "UFO\"1942\"", {{3, 5}, {4, 6}, {5, 7}}};

  std::cout << "__ Basic CoutWriter __" << std::endl;
  {
    std::stringstream out;
    out << polygon;
    TEST_EQ(out.str(), "Polygon{color:1, mood:2, name:\"UFO\\\"1942\\\"\", points:[Point{x:3, y:5}, Point{x:4, y:6}, Point{x:5, y:7}]}");
  }

  std::stringbuf buf;
  traverse::BinarySerialize serialize(buf);
  visit(serialize, polygon);

  {
    std::cout << "__ Serialize to bytes __ " << std::endl;
    TEST_EQ(to_bytes(polygon), "1 2 9 85 70 79 34 49 57 52 50 34 3 6 10 8 12 10 14 ");

    std::cout << "__ Deserialize from bytes __ " << std::endl;
    std::stringstream out1, out2;
    traverse::BinaryDeserialize reader(buf);
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
    std::stringbuf b;
    traverse::BinarySerialize s(b);
    visit(s, narrow);
    traverse::BinaryDeserialize u(b);
    visit(u, wide);
    TEST_EQ(narrow, wide);
    TEST_EQ(u.Errors(), "");
  }
    
  {
    std::cout << "__ Integer size shrunk __" << std::endl;
    uint64_t wide = 17291729;
    uint16_t narrow = 0xdead;
    std::stringbuf b;
    traverse::BinarySerialize s(b);
    visit(s, wide);
    traverse::BinaryDeserialize u(b);
    visit(u, narrow);
    TEST_EQ(narrow, wide & 0xffff);
    TEST_EQ(u.Errors(), "");
  }
    
  {
    std::cout << "__ Corrupt deserialize __ " << std::endl;
    std::string msg = buf.str();
    for (size_t i = 0; i < msg.size(); i++) {
      msg[i] = 0x7f;
    }
    std::stringbuf corrupted_buf(msg);
    traverse::BinaryDeserialize reader(corrupted_buf);
    Polygon polygon2;
    visit(reader, polygon2);
    TEST_EQ(reader.Errors().substr(0, 5), "Error");
  }

  {
    std::cout << "__ Serialized message too short __ " << std::endl;
    std::string msg = buf.str();
    msg.erase(msg.size()/2);
    std::stringbuf corrupted_buf(msg);
    traverse::BinaryDeserialize reader(corrupted_buf);
    Polygon polygon2;
    visit(reader, polygon2);
    TEST_EQ(reader.Errors().substr(0, 5), "Error");
  }

  // Extra bytes are not an error but can be detected by examining the streambuf
  {
    std::cout << "__ Serialized message too long __ " << std::endl;
    std::string msg = buf.str();
    msg += "12345";
    std::stringbuf corrupted_buf(msg);
    traverse::BinaryDeserialize reader(corrupted_buf);
    Polygon polygon2;
    visit(reader, polygon2);
    TEST_EQ(reader.Errors().substr(0, 5), "");
    TEST_EQ(corrupted_buf.in_avail(), 5);
  }
}
