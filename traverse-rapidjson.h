// Copyright 2017 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/**
 * Traverse-JSON extension using the rapidjson library.
 *
 * Example usage for C++ to JSON:
 *
 *     rapidjson::StringBuffer output;
 *     traverse::JsonWriter jsonwriter{output};
 *     visit(jsonwriter, yourobject);
 *     std::cout << buffer.GetString();
 *
 * Example usage for JSON to C++:
 * 
 *     rapidjson::Document input;
 *     input.Parse("json string");
 *     if (input.HasParseError()) { throw "parse error"; }
 *     std::stringstream errors;
 *     traverse::RapidJsonReader jsonreader{input, errors};
 *     visit(jsonreader, yourobject);
 *     if (!errors.empty()) { throw "read error"; }
 *
 */

#ifndef TRAVERSE_JSON_H
#define TRAVERSE_JSON_H

#define RAPIDJSON_HAS_STDSTRING 1

#include "traverse.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace traverse {

  /** The RapidJsonWriter will take a value from C++ and write it into
   *  a rapidjson stream (SAX).
   */
  struct RapidJsonWriter {
    rapidjson::StringBuffer& out;
    rapidjson::Writer<rapidjson::StringBuffer> writer;
    RapidJsonWriter(rapidjson::StringBuffer& out_): out(out_), writer(out) {}
  };

  template<typename T> inline
  typename std::enable_if<std::is_arithmetic<T>::value && !std::is_signed<T>::value, void>::type
  visit(RapidJsonWriter& writer, const T& value) {
    writer.writer.Uint64(value);
  }

  template<typename T> inline
  typename std::enable_if<std::is_arithmetic<T>::value && std::is_signed<T>::value, void>::type
  visit(RapidJsonWriter& writer, const T& value) {
    writer.writer.Int64(value);
  }

  template<> inline
  void visit(RapidJsonWriter& writer, const bool& value) {
    writer.writer.Bool(value);
  }

  template<> inline
  void visit(RapidJsonWriter& writer, const double& value) {
    writer.writer.Double(value);
  }

  template<typename T> inline
  typename std::enable_if<std::is_enum<T>::value, void>::type
  visit(RapidJsonWriter& writer, const T& value) {
    visit(writer, typename std::underlying_type<T>::type(value));
  }

  inline void visit(RapidJsonWriter& writer, const std::string& string) {
    writer.writer.String(string);
  }
  
  template<typename Element>
  void visit(RapidJsonWriter& writer, const std::vector<Element>& vector) {
    writer.writer.StartArray();
    for (auto element : vector) {
      visit(writer, element);
    }
    writer.writer.EndArray();
  }

  template<>
  struct StructVisitor<RapidJsonWriter> {
    RapidJsonWriter& writer;
    
    StructVisitor(const char*, RapidJsonWriter& writer_)
      : writer(writer_) {
      writer.writer.StartObject();
    }
    
    ~StructVisitor() {
      writer.writer.EndObject();
    }
    
    template<typename T>
    StructVisitor& field(const char* label, const T& value) {
      writer.writer.Key(label);
      visit(writer, value);
      return *this;
    }
  };


  /** The RapidJsonReader will take a rapidjson document and convert
   *  into a C++ object, leaving error messages in the user-supplied
   *  error stream.
   */
  struct RapidJsonReader {
    const rapidjson::Value& in;
    std::ostream& errors;
  };

  template<typename T> inline
  typename std::enable_if<std::is_arithmetic<T>::value && !std::is_signed<T>::value, void>::type
  visit(RapidJsonReader& reader, T& value) {
    if (!reader.in.IsUint64()) {
      reader.errors << "Warning: expected JSON uint; skipping" << std::endl;
      return;
    }
    value = T(reader.in.GetUint64());
  }

  template<typename T> inline
  typename std::enable_if<std::is_arithmetic<T>::value && std::is_signed<T>::value, void>::type
  visit(RapidJsonReader& reader, T& value) {
    if (!reader.in.IsInt64()) {
      reader.errors << "Warning: expected JSON int; skipping" << std::endl;
      return;
    }
    value = T(reader.in.GetInt64());
  }

  template<> inline
  void visit(RapidJsonReader& reader, double& value) {
    if (!reader.in.IsNumber()) {
      reader.errors << "Warning: expected JSON number; skipping" << std::endl;
      return;
    }
    value = reader.in.GetDouble();
  }

  template<> inline
  void visit(RapidJsonReader& reader, bool& value) {
    if (reader.in.IsBool()) {
      value = reader.in.GetBool();
    } else if (reader.in.IsNumber()) {
      value = reader.in.GetDouble() != 0.0;
    } else {
      reader.errors << "Warning: expected JSON bool or number; skipping" << std::endl;
    }
  }

  template<typename T> inline
  typename std::enable_if<std::is_enum<T>::value, void>::type
  visit(RapidJsonReader& reader, T& value) {
    typename std::underlying_type<T>::type v;
    visit(reader, v);
    value = T(v);
  }

  inline void visit(RapidJsonReader& reader, std::string& string) {
    if (!reader.in.IsString()) {
      reader.errors << "Warning: expected JSON string; skipping" << std::endl;
      return;
    }
    string = reader.in.GetString();
  }

  template<typename Element>
  void visit(RapidJsonReader& reader, std::vector<Element>& vector) {
    if (!reader.in.IsArray()) {
      reader.errors << "Warning: expected JSON array; skipping" << std::endl;
      return;
    }
    
    vector.clear();
    for (auto& json_element : reader.in.GetArray()) {
      vector.push_back(Element());
      RapidJsonReader element_reader{json_element, reader.errors};
      visit(element_reader, vector.back());
    }
  }

  template<>
  struct StructVisitor<RapidJsonReader> {
    const char* name;
    RapidJsonReader& reader;
    const rapidjson::Value& input;
    
    StructVisitor(const char* name_, RapidJsonReader& reader_)
      : name(name_),
        reader(reader_),
        input(reader.in)
    {
      if (!reader.in.IsObject()) {
        reader.errors << "Warning: expected JSON object; skipping" << std::endl;
      }
    }
    ~StructVisitor() {
    }
    
    template<typename T>
    StructVisitor& field(const char* label, T& value) {
      auto i = input.FindMember(label);
      if (i == input.MemberEnd()) {
        reader.errors << "Warning: JSON object missing field " << label << std::endl;
      } else {
        RapidJsonReader field_reader{i->value, reader.errors};
        visit(field_reader, value);
      }
      return *this;
    }
  };
  
}


#endif
