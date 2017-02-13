// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/**
 * Traverse-JSON extension using the picojson library.
 *
 * Example usage for C++ to JSON:
 *
 *     picojson::value output;
 *     traverse::PicoJsonWriter jsonwriter{output};
 *     visit(jsonwriter, yourobject);
 *     std::cout << output.serialize();
 *
 * Example usage for JSON to C++:
 * 
 *     picojson::value input;
 *     auto err = picojson::parse(input, "json string");
 *     if (!err.empty()) { throw "parse error"; }
 *     std::stringstream errors;
 *     traverse::PicoJsonReader jsonreader{input, errors};
 *     visit(jsonreader, yourobject);
 *     if (!errors.empty()) { throw "read error"; }
 *
 */

#ifndef TRAVERSE_JSON_H
#define TRAVERSE_JSON_H

#include "traverse.h"
#include "picojson/picojson.h"

namespace traverse {

  /** The PicoJsonWriter will take a value from C++ and write it into
   *  a picojson object.
   */
  struct PicoJsonWriter {
    picojson::value& out;
    PicoJsonWriter(picojson::value& out_): out(out_) {}
  };

  template<typename T> inline
  typename std::enable_if<std::is_arithmetic<T>::value, void>::type
  visit(PicoJsonWriter& writer, const T& value) {
    writer.out = picojson::value(double(value));
  }

  template<typename T> inline
  typename std::enable_if<std::is_enum<T>::value, void>::type
  visit(PicoJsonWriter& writer, const T& value) {
    visit(writer, typename std::underlying_type<T>::type(value));
  }

  inline void visit(PicoJsonWriter& writer, const std::string& string) {
    writer.out = picojson::value(string);
  }
  
  template<typename Element>
  void visit(PicoJsonWriter& writer, const std::vector<Element>& vector) {
    picojson::value::array output;
    for (auto element : vector) {
      output.push_back(picojson::value());
      PicoJsonWriter element_writer{output.back()};
      visit(element_writer, element);
    }
    writer.out = picojson::value(output);
  }

  template<>
  struct StructVisitor<PicoJsonWriter> {
    const char* name;
    PicoJsonWriter& writer;
    picojson::value::object output;
    
    StructVisitor(const char* name_, PicoJsonWriter& writer_)
      : name(name_), writer(writer_) {}
    ~StructVisitor() {
      writer.out = picojson::value(output);
    }
    
    template<typename T>
    StructVisitor& field(const char* label, const T& value) {
      output[label] = picojson::value();
      PicoJsonWriter field_writer{output[label]};
      visit(field_writer, value);
      return *this;
    }
  };


  /** The PicoJsonReader will take a picojson object and convert
   *  into a C++ object, leaving error messages in the user-supplied
   *  error stream.
   */
  struct PicoJsonReader {
    const picojson::value& in;
    std::ostream& errors;
  };

  template<typename T> inline
  typename std::enable_if<std::is_arithmetic<T>::value, void>::type
  visit(PicoJsonReader& reader, T& value) {
    if (!reader.in.is<double>()) {
      reader.errors << "Warning: expected JSON number; skipping" << std::endl;
      return;
    }
    value = T(reader.in.get<double>());
  }

  template<typename T> inline
  typename std::enable_if<std::is_enum<T>::value, void>::type
  visit(PicoJsonReader& reader, T& value) {
    typename std::underlying_type<T>::type v;
    visit(reader, v);
    value = T(v);
  }

  inline void visit(PicoJsonReader& reader, std::string& string) {
    if (!reader.in.is<std::string>()) {
      reader.errors << "Warning: expected JSON string; skipping" << std::endl;
      return;
    }
    string = reader.in.get<std::string>();
  }

  template<typename Element>
  void visit(PicoJsonReader& reader, std::vector<Element>& vector) {
    if (!reader.in.is<picojson::value::array>()) {
      reader.errors << "Warning: expected JSON array; skipping" << std::endl;
      return;
    }
    
    const picojson::value::array& array = reader.in.get<picojson::value::array>();
    vector.clear();
    for (auto json_element : array) {
      vector.push_back(Element());
      PicoJsonReader element_reader{json_element, reader.errors};
      visit(element_reader, vector.back());
    }
  }

  template<>
  struct StructVisitor<PicoJsonReader> {
    const char* name;
    PicoJsonReader& reader;
    const picojson::value::object& input;
    
    StructVisitor(const char* name_, PicoJsonReader& reader_)
      : name(name_),
        reader(reader_),
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
        PicoJsonReader field_reader{i->second, reader.errors};
        visit(field_reader, value);
      }
      return *this;
    }
  };
  
}


#endif
