// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "traverse.h"
#include "traverse-json.h"
#include <iostream>
#include "test.h"


int main() {
  const Point p = {3, 5};
  const LineSegment s{{1, 7}, {13, 19}};
  const Polygon polygon = {BLUE, Mood::HULK_SMASH, "UFO\"1942\"", {{3, 5}, {4, 6}, {5, 7}}};
  
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
}
