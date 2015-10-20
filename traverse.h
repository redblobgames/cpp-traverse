// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef TRAVERSE_H
#define TRAVERSE_H

#include <iostream>
#include <sstream>
#include <vector>

namespace traverse {
  // For each visitor you need a type, a visit() function, and one
  // handle() function per primitive
  
  struct CoutWriter {
  };

  template<typename T>
  void visit(CoutWriter& writer, const char* label, T& value) {
    std::cout << "(" << label << ":";
    handle(writer, value);
    std::cout << ")";
  }

  template<class T> inline
  typename std::enable_if<std::is_arithmetic<T>::value, void>::type
  handle(CoutWriter& writer, T& value) {
    std::cout << value;
  }

  template<typename Element>
  void handle(CoutWriter& visitor, std::vector<Element>& vector) {
    for (int i = 0; i < vector.size(); ++i) {
      // HACK: until I write generic labels
      char* label = new char[3];
      sprintf(label, "%d", i);
      // HACK: ends
      
      visit(visitor, label, vector[i]);
    }
  }

  
  // Binary serialize

  struct BinarySerialize {
    std::stringbuf out;
    BinarySerialize(): out(std::ios_base::out) {}
  };

  template<typename T> inline
  void visit(BinarySerialize& writer, const char* label, T& value) {
    handle(writer, value);
  }

  template<class T> inline
  typename std::enable_if<std::is_integral<T>::value, void>::type
  handle(BinarySerialize& writer, T& value) {
    writer.out.sputn(reinterpret_cast<const char *>(&value), sizeof(T));
  }

  template<typename Element>
  void handle(BinarySerialize& writer, std::vector<Element>& vector) {
    uint32_t size = vector.size();
    writer.out.sputn(reinterpret_cast<const char*>(&size), sizeof(size));
    for (auto& element : vector) {
      visit(writer, "#null", element);
    }
  }

  // Binary deserialize

  struct BinaryDeserialize {
    std::stringbuf in;
    BinaryDeserialize(const std::string& str): in(str, std::ios_base::in) {}
  };

  template<typename T> inline
  void visit(BinaryDeserialize& reader, const char* label, T& value) {
    handle(reader, value);
  }

  template<class T> inline
  typename std::enable_if<std::is_integral<T>::value, void>::type
  handle(BinaryDeserialize& reader, T& value) {
    reader.in.sgetn(reinterpret_cast<char *>(&value), sizeof(T));
  }

  template<typename Element>
  void handle(BinaryDeserialize& reader, std::vector<Element>& vector) {
    uint32_t size = vector.size();
    reader.in.sgetn(reinterpret_cast<char*>(&size), sizeof(size));
    uint32_t i = 0;
    for (; i < size && reader.in.in_avail() > 0; ++i) {
      Element element;
      visit(reader, "#null", element);
      vector.push_back(std::move(element));
    }
    if (i != size) {
      std::cerr << "Warning: expected " << size << " elements in vector but only found " << i << std::endl;
    }
  }

  
  // Generic traversals
  template<typename Visitor>
  struct visit_struct_ {
    Visitor& visitor;
    template<typename T>
    visit_struct_& operator ()(const char* label, T& value) {
      visit(visitor, label, value);
      return *this;
    }
  };
  template<typename Visitor>
  visit_struct_<Visitor> visit_struct(Visitor& visitor) { return visit_struct_<Visitor>{visitor}; }


  // TODO: move labels up one level so that vectors don't need labels,
  // and the begin/end of a struct or vector can be written out (size
  // for binary, delimiters for json)

  // TODO: vector handling should be per visitor type, not a generic one
}


#endif
