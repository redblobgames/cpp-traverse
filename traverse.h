// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef TRAVERSE_H
#define TRAVERSE_H

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#define TRAVERSE_STRUCT(TYPE, FIELDS) namespace traverse { template<typename Visitor> void visit(Visitor& visitor, TYPE& obj) { visit_struct(visitor) FIELDS ; } template<typename Visitor> void visit(Visitor& visitor, const TYPE& obj) { visit_struct(visitor) FIELDS ; } }
#define FIELD(NAME) .field(#NAME, obj.NAME)

namespace traverse {

  /* This is how user-defined structs are described to the system:
   * 
   * TRAVERSE_STRUCT(MyUserType, FIELD(x) FIELD(y))
   * 
   * That macro turns into
   *
   * template<typename Visitor>
   * void visit(Visitor& visitor, MyUserType& obj) {
   *   visit_struct(visitor)
   *      .field("x", obj.x)
   *      .field("y", obj.y);
   * }
   *
   * as well as a const MyUserType& version.
   *
   * The visit_struct function constructs a local
   * StructVisitor<Visitor> object, which is destroyed when the
   * struct's visit() function returns. Each visitor type can
   * specialize this as needed to keep local fields or do work in the
   * constructor and destructor.
   *
   * If some fields aren't public, declare the visit function to be a
   * friend:
   *
   * template <typename V>
   *    friend void traverse::visit(V&, MyUserType&);
   * template <typename V>
   *    friend void traverse::visit(V&, const MyUserType&);
   *
   */
  
  template<typename Visitor>
  struct StructVisitor {
    Visitor& visitor;
    template<typename T>
    StructVisitor& field(const char* label, T& value) {
      visit(visitor, value);
      return *this;
    }
  };
  
  template<typename Visitor>
  StructVisitor<Visitor> visit_struct(Visitor& visitor) {
    return StructVisitor<Visitor>{visitor};
  }

  /* Each visitor type needs visit() functions for the standard types
   * it handles (primitives, strings, vectors) and optionally a
   * StructVisitor to handle the field name/value pairs in a struct.
   */
}


/* The CoutWriter just writes everything to std::cout */

namespace traverse {
  struct CoutWriter {
  };

  template<class T> inline
  typename std::enable_if<std::is_arithmetic<T>::value, void>::type
  visit(CoutWriter& writer, const T& value) {
    std::cout << value;
  }

  void visit(CoutWriter& visitor, const std::string& string) {
    std::cout << '(' << string << ')';
  }
  
  template<typename Element>
  void visit(CoutWriter& visitor, const std::vector<Element>& vector) {
    std::cout << '[';
    for (int i = 0; i < vector.size(); ++i) {
      if (i != 0) std::cout << ", ";
      visit(visitor, vector[i]);
    }
    std::cout << ']';
  }

  template<>
  struct StructVisitor<CoutWriter> {
    CoutWriter& visitor;
    bool first;
    
    StructVisitor(CoutWriter& visitor_): visitor(visitor_), first(true) {
      std::cout << '{';
    }

    ~StructVisitor() {
      std::cout << '}';
    }
    
    template<typename T>
    StructVisitor& field(const char* label, const T& value) {
      if (!first) std::cout << ", ";
      first = false;
      std::cout << label << ':';
      visit(visitor, value);
      return *this;
    }
  };
}


/* The binary serialize/deserialize uses a binary format and stringbufs */

namespace traverse {

  struct BinarySerialize {
    std::stringbuf out;
    BinarySerialize(): out(std::ios_base::out) {}
  };

  template<class T> inline
  typename std::enable_if<std::is_integral<T>::value, void>::type
  visit(BinarySerialize& writer, const T& value) {
    writer.out.sputn(reinterpret_cast<const char *>(&value), sizeof(T));
  }

  void visit(BinarySerialize& writer, const std::string& string) {
    uint32_t size = string.size();
    writer.out.sputn(reinterpret_cast<const char*>(&size), sizeof(size));
    writer.out.sputn(&string[0], size);
  }
  
  template<typename Element>
  void visit(BinarySerialize& writer, const std::vector<Element>& vector) {
    uint32_t size = vector.size();
    writer.out.sputn(reinterpret_cast<const char*>(&size), sizeof(size));
    for (auto& element : vector) {
      visit(writer, element);
    }
  }


  struct BinaryDeserialize {
    std::stringbuf in;
    std::stringstream errors;
    BinaryDeserialize(const std::string& str): in(str, std::ios_base::in) {}
  };

  template<class T> inline
  typename std::enable_if<std::is_integral<T>::value, void>::type
  visit(BinaryDeserialize& reader, T& value) {
    if (reader.in.sgetn(reinterpret_cast<char *>(&value), sizeof(T)) < sizeof(T)) {
      reader.errors << "Error: not enough data in buffer to read number" << std::endl;
    }
  }

  void visit(BinaryDeserialize& reader, std::string& string) {
    uint32_t size = 0;
    if (reader.in.sgetn(reinterpret_cast<char*>(&size), sizeof(size)) < sizeof(size)) {
      reader.errors << "Error: not enough data in buffer to read string size" << std::endl;
      return;
    }
    if (reader.in.in_avail() < size) {
      reader.errors << "Error: not enough data in buffer to deserialize string" << std::endl;
      return;
    }
    string.resize(size);
    reader.in.sgetn(&string[0], size);
  }
  
  template<typename Element>
  void visit(BinaryDeserialize& reader, std::vector<Element>& vector) {
    uint32_t size = 0;
    if (reader.in.sgetn(reinterpret_cast<char*>(&size), sizeof(size)) < sizeof(size)) {
      reader.errors << "Error: not enough data in buffer to read vector size" << std::endl;
      return;
    }
    uint32_t i = 0;
    vector.clear();
    for (; i < size && reader.in.in_avail() > 0; ++i) {
      Element element;
      visit(reader, element);
      vector.push_back(std::move(element));
    }
    if (i != size) {
      reader.errors << "Error: expected " << size << " elements in vector but only found " << i << std::endl;
    }
  }
}


#endif
