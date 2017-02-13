// Copyright 2017 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "traverse.h"
#include "traverse-rapidjson.h"
#include <iostream>
#include "test.h"

using std::string;

template<typename T>
void test_serialize(T from_value, string to_value) {
  rapidjson::StringBuffer json;
  traverse::RapidJsonWriter jsonwriter{json};
  visit(jsonwriter, from_value);
  TEST_EQ(string(json.GetString()), to_value);
}

template<typename T>
void test_deserialize(string from_json, T to_value) {
  rapidjson::Document json;
  std::stringstream errors;
  json.Parse(from_json);
  traverse::RapidJsonReader jsonreader{json, errors};
  T output;
  visit(jsonreader, output);
  TEST_EQ(output, to_value);
  TEST_EQ_QUIET(errors.str(), "");
}

template<typename T>
void test_deserialize_fail(string from_json) {
  rapidjson::Document json;
  std::stringstream errors;
  json.Parse(from_json);
  traverse::RapidJsonReader jsonreader{json, errors};
  T output;
  visit(jsonreader, output);
  TEST_EQ(errors.str().substr(0, 7), "Warning");
}

template<typename T>
void test_both(T native_value, string json_value) {
  test_serialize(native_value, json_value);
  test_deserialize(json_value, native_value);
}

void test_bools() {
  std::cout << "__ Test bools __\n";

  test_both(false, "false");
  test_both(true, "true");

  test_deserialize("0", false);
  test_deserialize("1", true);
  test_deserialize("-1", true);
  test_deserialize("0.0", false);
  test_deserialize("1.0", true);
  test_deserialize_fail<bool>("null");
  test_deserialize_fail<bool>("\"string\"");
  test_deserialize_fail<bool>("{\"object\"}");
  test_deserialize_fail<bool>("[\"array\"]");
}

void test_ints() {
  std::cout << "__ Test ints __\n";

  test_both(0, "0");
  test_both(5, "5");
  test_both(-3, "-3");
  test_both(char(64), "64");
  test_both(uint16_t(0xffff), "65535");
  test_both(uint32_t(0xffffffff), "4294967295");
  test_both(uint64_t((1ul << 53) - 1), "9007199254740991"); // max int in javascript
  test_both(uint64_t(0xffffffffffffffff), "18446744073709551615"); // max uint64_t
  test_both(int16_t(0xffff), "-1");
  test_both(int32_t(0xffffffff), "-1");
  test_both(int64_t(0xffffffffffffffff), "-1");

  test_deserialize_fail<int>("1.3");
  test_deserialize_fail<uint64_t>("1.3");
  test_deserialize_fail<uint64_t>("-3");
  test_deserialize_fail<int>("null");
  test_deserialize_fail<int>("\"string\"");
  test_deserialize_fail<int>("{\"object\"}");
  test_deserialize_fail<int>("[\"array\"]");
}

void test_doubles() {
  std::cout << "__ Test doubles __\n";
  
  test_both(0.0, "0.0");
  test_both(1.0, "1.0");
  test_both(2.5, "2.5");
  test_both(123456.789, "123456.789");
  test_both(-314.89, "-314.89");
  test_both(1e50, "1e50");
  test_deserialize("1", 1.0);
  test_deserialize_fail<double>("null");
  test_deserialize_fail<double>("\"string\"");
  test_deserialize_fail<double>("{\"object\"}");
  test_deserialize_fail<double>("[\"array\"]");
}

int main() {
  test_bools();
  test_ints();
  test_doubles();
  
  const LineSegment s{{1, 7}, {13, 19}};
  const Polygon polygon = {BLUE, Mood::HULK_SMASH, "UFO\"1942\"", {{3, 5}, {4, 6}, {5, 7}}};
  
  {
    std::cout << "__ Serialize object to JSON __\n";
    rapidjson::StringBuffer json;
    traverse::RapidJsonWriter jsonwriter{json};
    visit(jsonwriter, polygon);
    TEST_EQ(string(json.GetString()), string("{\"color\":1,\"mood\":2,\"name\":\"UFO\\\"1942\\\"\",\"points\":[{\"x\":3,\"y\":5},{\"x\":4,\"y\":6},{\"x\":5,\"y\":7}]}"));
  }

  // Intentionally make the JSON mismatch the C++ data structure, to make sure it flags warnings.
  rapidjson::Document json2;
  {
    std::cout << "__ Deserialize from JSON to RapidJson __\n";
    json2.Parse("{\"points\":[{\"UNUSED\":0,\"x\":3,\"y\":5},{\"y\":6,\"x\":4},{\"y\":7},{\"x\":\"WRONGTYPE\"}]}");
    TEST_EQ(json2.HasParseError(), false);
    
    std::cout << "__ Deserialize JSON to Polygon __\n";
    std::stringstream errors;
    traverse::RapidJsonReader jsonreader{json2, errors};
    Polygon polygon2;
    visit(jsonreader, polygon2);

    // The resulting polygon should have the x and y fields set even
    // when other fields are missing or extraneous. Test both that and
    // the non-empty errors string.
    std::stringstream out;
    out << polygon2;
    TEST_EQ(out.str(), "Polygon{color:0, mood:0, name:\"\", points:[Point{x:3, y:5}, Point{x:4, y:6}, Point{x:0, y:7}, Point{x:0, y:0}]}");
    TEST_EQ(errors.str().substr(0, 7), "Warning");
  }
}
