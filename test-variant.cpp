// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "traverse.h"
#include "traverse-variant.h"
#include "variant-util.h"

#include "mapbox/variant.hpp"
template<typename ...T> using variant = mapbox::util::variant<T...>;

#include <type_traits>
#include <functional>
#include <sstream>
#include <vector>

#include "test.h"

struct Move {
  int speed;
  int turn;
};
TRAVERSE_STRUCT(Move, FIELD(speed) FIELD(turn))

struct Create {
  int id;
  int x, y;
};
TRAVERSE_STRUCT(Create, FIELD(id) FIELD(x) FIELD(y))

struct Quit {
  int time;
};
TRAVERSE_STRUCT(Quit, FIELD(time))

using Message = variant<Create, Move, Quit>;
using MessageQueue = std::vector<Message>;


void test_match_function() {
  std::stringstream output;
  traverse::CoutWriter debug(output);
  Message m1{Move{1, 2}};
  Message m2{Create{42, -10, -10}};
  Message m3{Quit{100}};
  MessageQueue queue{m1, m2, m3};

  for (auto& msg : queue) {
    msg.match(
          [&](const Move& m) {
            output << "Move ";
            visit(debug, m);
          },
          [&](const Create& m) {
            output << "Create ";
            visit(debug, m);
          },
          [&](const Quit& m) {
            output << "Quit ";
            visit(debug, m);
          }
          );
  }

  TEST_EQ(output.str(), "Move Move{speed:1, turn:2}Create Create{id:42, x:-10, y:-10}Quit Quit{time:100}");
}


void test_serialization() {
  Message m1{Move{1, 2}};
  Message m2{Create{42, -10, -10}};
  MessageQueue queue{m1, m2};

  // Test a round trip; make sure it comes back the same
  {
    std::stringbuf buf;
    traverse::BinarySerialize serialize(buf);
    visit(serialize, queue);

    MessageQueue another_queue;
    traverse::BinaryDeserialize deserialize(buf);
    visit(deserialize, another_queue);

    std::stringstream before, after;
    traverse::CoutWriter write_before(before), write_after(after);
    visit(write_before, queue);
    visit(write_after, queue);
    TEST_EQ(before.str(), after.str());
  }
  
  // Test corrupting the data
  {
    Message m;
    std::stringbuf buf;
    traverse::BinarySerialize serialize(buf);
    visit(serialize, m1);
  
    std::string msg = buf.str();
    msg[0] = 0;  // this will make it pick the wrong variant
    std::stringbuf corrupted_buf1(msg);
    traverse::BinaryDeserialize wrong_variant(corrupted_buf1);
    visit(wrong_variant, m);
    TEST_EQ(wrong_variant.Errors().size() != 0, true);

    msg[0] = -5;  // this does not correspond to any valid variant
    std::stringbuf corrupted_buf2(msg);
    traverse::BinaryDeserialize invalid_type(corrupted_buf2);
    visit(invalid_type, m);
    TEST_EQ(wrong_variant.Errors().size() != 0, true);
  }
}



int main() {
  test_match_function();
  test_serialization();
}
