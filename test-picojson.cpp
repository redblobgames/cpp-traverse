// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "traverse.h"
#include "traverse-picojson.h"
#include <iostream>
#include "test.h"

// TODO: there should be a lot more tests here, more like test-lua.cpp 

int main() {
  const Polygon polygon = {BLUE, Mood::HULK_SMASH, Charred::END, "UFO\"1942\"", {{3, 5}, {4, 6}, {5, 7}}};
  
  {
    std::cout << "__ Serialize to JSON __ " << std::endl;
    picojson::value json1;
    traverse::PicoJsonWriter jsonwriter{json1};
    visit(jsonwriter, polygon);
    TEST_EQ(json1.serialize(), "{\"charred\":1,\"color\":1,\"mood\":2,\"name\":\"UFO\\\"1942\\\"\",\"points\":[{\"x\":3,\"y\":5},{\"x\":4,\"y\":6},{\"x\":5,\"y\":7}]}");
  }

  // Intentionally make the JSON mismatch the C++ data structure, to make sure it flags warnings.
  picojson::value json2;
  {
    std::cout << "__ Deserialize from JSON to PicoJSON __ " << std::endl;
    auto err = picojson::parse(json2, "{\"points\":[{\"UNUSED\":0,\"x\":3,\"y\":5},{\"y\":6,\"x\":4},{\"y\":7},{\"x\":\"WRONGTYPE\"}]}");
    TEST_EQ(err, "");
    
    std::cout << "__ Deserialize JSON to Polygon __ " << std::endl;
    std::stringstream errors;
    traverse::PicoJsonReader jsonreader{json2, errors};
    Polygon polygon2{}; // default values
    visit(jsonreader, polygon2);

    // The resulting polygon should have the x and y fields set even
    // when other fields are missing or extraneous. Test both that and
    // the non-empty errors string.
    std::stringstream out;
    out << polygon2;
    TEST_EQ(out.str(), "Polygon{color:0, mood:0, charred:0, name:\"\", points:[Point{x:3, y:5}, Point{x:4, y:6}, Point{x:0, y:7}, Point{x:0, y:0}]}");
    TEST_EQ(errors.str().substr(0, 7), "Warning");
  }
}
