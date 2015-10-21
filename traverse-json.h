// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef TRAVERSE_JSON_H
#define TRAVERSE_JSON_H

#include "traverse.h"
#include "picojson/picojson.h"

/* JSON output:
 *
 * picojson::value json;
 * traverse::JsonWriter jsonwriter{json};
 * visit(jsonwriter, yourobject);
 * std::cout << json.serialize();
 *
 *
 * JSON input:
 * 
 * picojson::value json;
 * auto err = picojson::parse(json, "json string");
 * if (!err.empty()) { throw "parse error"; }
 * traverse::JsonReader jsonreader{json};
 * visit(jsonreader, yourobject);
 *
 * TODO: wrapper around this code
 */
namespace traverse {
  
  struct JsonWriter {
    picojson::value& out;
    JsonWriter(picojson::value& out_): out(out_) {}
  };

  template<class T> inline
  typename std::enable_if<std::is_arithmetic<T>::value, void>::type
  visit(JsonWriter& writer, T& value) {
    writer.out = std::move(picojson::value(double(value)));
  }

  template<typename Element>
  void visit(JsonWriter& writer, std::vector<Element>& vector) {
    picojson::value::array output;
    for (auto element : vector) {
      output.push_back(picojson::value());
      JsonWriter element_writer{output.back()};
      visit(element_writer, element);
    }
    writer.out = std::move(picojson::value(output));
  }

  template<>
  struct StructVisitor<JsonWriter> {
    JsonWriter& writer;
    picojson::value::object output;
    
    StructVisitor(JsonWriter& writer_): writer(writer_) {}
    ~StructVisitor() {
      writer.out = std::move(picojson::value(output));
    }
    
    template<typename T>
    StructVisitor& operator ()(const char* label, T& value) {
      output[label] = picojson::value();
      JsonWriter field_writer{output[label]};
      visit(field_writer, value);
      return *this;
    }
  };


  struct JsonReader {
    const picojson::value& in;
  };

  template<class T> inline
  typename std::enable_if<std::is_arithmetic<T>::value, void>::type
  visit(JsonReader& reader, T& value) {
    if (!reader.in.is<double>()) {
      std::cerr << "JSON expected number" << std::endl;
      return;
    }
    value = T(reader.in.get<double>());
  }

  template<typename Element>
  void visit(JsonReader& reader, std::vector<Element>& vector) {
    if (!reader.in.is<picojson::value::array>()) {
      std::cerr << "JSON expected array" << std::endl;
      return;
    }
    
    const picojson::value::array& array = reader.in.get<picojson::value::array>();
    vector.clear();
    for (auto json_element : array) {
      vector.push_back(Element());
      JsonReader element_reader{json_element};
      visit(element_reader, vector.back());
    }
  }

  template<>
  struct StructVisitor<JsonReader> {
    JsonReader& reader;
    const picojson::value::object& input;
    
    StructVisitor(JsonReader& reader_)
      : reader(reader_),
        input(reader.in.get<picojson::value::object>())
    {
      if (!reader.in.is<picojson::value::object>()) {
        std::cerr << "JSON expected object" << std::endl;
      }
    }
    ~StructVisitor() {
    }
    
    template<typename T>
    StructVisitor& operator ()(const char* label, T& value) {
      std::string key(label);
      auto i = input.find(key);
      if (i == input.end()) {
        std::cerr << "JSON object missing field " << key << std::endl;
      } else {
        JsonReader field_reader{i->second};
        visit(field_reader, value);
      }
      return *this;
    }
  };
  
}


#endif
