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
 * std::stringstream errors;
 * traverse::JsonReader jsonreader{json, errors};
 * if (!errors.empty()) { throw "read error"; }
 * visit(jsonreader, yourobject);
 *
 * TODO: wrapper around this code
 */
namespace traverse {
  
  struct JsonWriter {
    picojson::value& out;
    JsonWriter(picojson::value& out_): out(out_) {}
  };

  template<typename T> inline
  typename std::enable_if<is_enum_or_number<T>::value, void>::type
  visit(JsonWriter& writer, const T& value) {
    writer.out = std::move(picojson::value(double(value)));
  }

  inline void visit(JsonWriter& writer, const std::string& string) {
    writer.out = std::move(picojson::value(string));
  }
  
  template<typename Element>
  void visit(JsonWriter& writer, const std::vector<Element>& vector) {
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
    StructVisitor& field(const char* label, const T& value) {
      output[label] = picojson::value();
      JsonWriter field_writer{output[label]};
      visit(field_writer, value);
      return *this;
    }
  };


  struct JsonReader {
    const picojson::value& in;
    std::ostream& errors;
  };

  template<typename T> inline
  typename std::enable_if<is_enum_or_number<T>::value, void>::type
  visit(JsonReader& reader, T& value) {
    if (!reader.in.is<double>()) {
      reader.errors << "Warning: expected JSON number; skipping" << std::endl;
      return;
    }
    value = T(reader.in.get<double>());
  }

  inline void visit(JsonReader& reader, std::string& string) {
    if (!reader.in.is<std::string>()) {
      reader.errors << "Warning: expected JSON string; skipping" << std::endl;
      return;
    }
    string = reader.in.get<std::string>();
  }

  template<typename Element>
  void visit(JsonReader& reader, std::vector<Element>& vector) {
    if (!reader.in.is<picojson::value::array>()) {
      reader.errors << "Warning: expected JSON array; skipping" << std::endl;
      return;
    }
    
    const picojson::value::array& array = reader.in.get<picojson::value::array>();
    vector.clear();
    for (auto json_element : array) {
      vector.push_back(Element());
      JsonReader element_reader{json_element, reader.errors};
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
        reader.errors << "Warning: expected JSON object; skipping" << std::endl;
      }
    }
    ~StructVisitor() {
    }
    
    template<typename T>
    StructVisitor& field(const char* label, T& value) {
      std::string key(label);
      auto i = input.find(key);
      if (i == input.end()) {
        reader.errors << "Warning: JSON object missing field " << key << std::endl;
      } else {
        JsonReader field_reader{i->second, reader.errors};
        visit(field_reader, value);
      }
      return *this;
    }
  };
  
}


#endif
