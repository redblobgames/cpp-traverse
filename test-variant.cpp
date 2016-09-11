// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "traverse.h"
#include "traverse-variant.h"
#include "variant-util.h"

#include "variant/variant.hpp"
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
    match(msg,
          [&](const Move& m) {
            output << "Move ";
            visit(debug, m);
          },
          [&](const Create& m) {
            output << "Create ";
            visit(debug, m);
          },
          /* Message types can be in here multiple times */
          [&](const Move& m) {
            output << "MoveAgain ";
          }
          /* No case handles the Quit message */
          );
  }

  TEST_EQ(output.str(), "Move Move{speed:1, turn:2}MoveAgain Create Create{id:42, x:-10, y:-10}");
}


void test_serialization() {
  Message m1{Move{1, 2}};
  Message m2{Create{42, -10, -10}};
  MessageQueue queue{m1, m2};

  // Test a round trip; make sure it comes back the same
  {
    traverse::BinarySerialize serialize;
    visit(serialize, queue);

    MessageQueue another_queue;
    traverse::BinaryDeserialize deserialize(serialize.out.str());
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
    traverse::BinarySerialize serialize;
    serialize.out.str().clear();
    visit(serialize, m1);
  
    std::string msg = serialize.out.str();
    msg[0] = 0;  // this will make it pick the wrong variant
    traverse::BinaryDeserialize wrong_variant(msg);
    visit(wrong_variant, m);
    TEST_EQ(wrong_variant.Errors().size() != 0, true);

    msg[0] = -5;  // this does not correspond to any valid variant
    traverse::BinaryDeserialize invalid_type(msg);
    visit(invalid_type, m);
    TEST_EQ(wrong_variant.Errors().size() != 0, true);
  }
}



int main() {
  test_match_function();
  test_serialization();
}
