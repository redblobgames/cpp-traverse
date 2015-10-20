// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef TRAVERSE_JSON_H
#define TRAVERSE_JSON_H

#include "traverse.h"
#include "picojson/picojson.h"

namespace traverse {
  struct JsonWriter {
    picojson::value& out;
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
  
}


#endif
