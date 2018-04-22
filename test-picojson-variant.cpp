// Copyright 2018 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "traverse.h"
#include "traverse-picojson.h"
#include "traverse-variant.h"
#include "traverse-picojson-variant.h"
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


void test_serialization() {
  const auto JSON_DATA = "[{\"data\":{\"speed\":1,\"turn\":2},\"which\":1},{\"data\":{\"id\":42,\"x\":-10,\"y\":-10},\"which\":0}]";

  Message m1{Move{1, 2}};
  Message m2{Create{42, -10, -10}};
  MessageQueue queue{m1, m2};

  // Test writing to json
  {
    std::cout << "__ Serialize to PicoJSON __ " << std::endl;
    picojson::value json1;
    traverse::PicoJsonWriter jsonwriter{json1};
    visit(jsonwriter, queue);
    TEST_EQ(json1.serialize(), JSON_DATA);
  }

  // Test reading from json
  {
    picojson::value json2;
    std::cout << "__ Deserialize from PicoJSON __ " << std::endl;
    auto err = picojson::parse(json2, JSON_DATA);
    TEST_EQ(err, "");
    
    std::cout << "__ Deserialize JSON to MessageQueue __ " << std::endl;
    std::stringstream errors;
    traverse::PicoJsonReader jsonreader{json2, errors};
    MessageQueue queue2;
    visit(jsonreader, queue2);

    std::stringstream before, after;
    traverse::CoutWriter write_before(before), write_after(after);
    visit(write_before, queue);
    visit(write_after, queue2);
    TEST_EQ(before.str(), after.str());
  }

  // TODO: test handling of corrupted json
}



int main() {
  test_serialization();
}
