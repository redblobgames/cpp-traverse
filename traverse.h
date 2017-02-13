// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/**
 * The traverse library traverses a C++ data structure recursively, 
 * applying some operation to each element. Included operations are
 *
 *   - debugging output using ostream::operator <<
 *   - binary serialization
 *   - binary deserialization
 *
 * The serialization format offers no backwards/forwards compatibility.
 * It is useful for network messages between client and server, but not
 * for save files or other uses of serialization.
 *
 * The traverse library is generic and can be extended to more data types
 * and also more operations. To extend it to work on a user-defined struct
 * or class, see TRAVERSE_STRUCT below. To extend it to work on a container,
 * see traverse-variant.h, which extends traverse to work on mapbox::variant.
 * To extend it to a new operation, see traverse-json.h, which writes to or 
 * reads from a JSON object (via the picojson library), and traverse-lua.h, 
 * which writes to or reads from a Lua object on the Lua stack.
 */

#ifndef TRAVERSE_H
#define TRAVERSE_H

#include <iostream>
#include <iomanip>
#include <streambuf>
#include <sstream>
#include <vector>
#include <string>

namespace traverse {

  /* This is how user-defined structs are described to the system:
   * 
   * TRAVERSE_STRUCT(MyUserType, FIELD(x) FIELD(y))
   * 
   * That macro turns into
   *
   *     template<typename Visitor>
   *     void visit(Visitor& visitor, MyUserType& obj) {
   *       visit_struct("MyUserType", visitor)
   *          .field("x", obj.x)
   *          .field("y", obj.y);
   *     }
   *
   * as well as a const MyUserType& version.
   *
   * The visit_struct function constructs a local
   * StructVisitor<Visitor> object, which is destroyed when the
   * struct's visit() function returns. Each visitor type can
   * specialize this as needed to keep local fields or do work in the
   * constructor and destructor.
   *
   * If some fields aren't public, put this inside your class:
   * 
   *     TRAVERSE_IS_FRIEND(MyUserType)
   *
   * That macro turns into
   * 
   *     template <typename V>
   *        friend void traverse::visit(V&, MyUserType&);
   *     template <typename V>
   *        friend void traverse::visit(V&, const MyUserType&);
   *
   */
  
  template<typename Visitor>
  struct StructVisitor {
    const char* name;
    Visitor& visitor;
    template<typename T>
    StructVisitor& field(const char* label, T& value) {
      visit(visitor, value);
      return *this;
    }
  };
  
  template<typename Visitor>
  StructVisitor<Visitor> visit_struct(const char* name, Visitor& visitor) {
    return StructVisitor<Visitor>{name, visitor};
  }

  /* Each visitor type needs visit() functions for the standard types
   * it handles (primitives, strings, vectors) and optionally a
   * StructVisitor to handle the field name/value pairs in a struct.
   */
}


#define TRAVERSE_STRUCT(TYPE, FIELDS) namespace traverse { template<typename Visitor> void visit(Visitor& visitor, TYPE& obj) { visit_struct(#TYPE, visitor) FIELDS ; } template<typename Visitor> void visit(Visitor& visitor, const TYPE& obj) { visit_struct(#TYPE, visitor) FIELDS ; } } inline std::ostream& operator << (std::ostream& out, const TYPE& obj) { traverse::CoutWriter writer(out); visit(writer, obj); return out; }
#define FIELD(NAME) .field(#NAME, obj.NAME)
#define TRAVERSE_IS_FRIEND(TYPE) template <typename V> friend void traverse::visit(V&, TYPE&); template <typename V> friend void traverse::visit(V&, const TYPE&);


/* The CoutWriter just writes everything to std::cout */

namespace traverse {
  struct CoutWriter {
    std::ostream& out;
    CoutWriter(std::ostream& out_ = std::cout) : out(out_) {}
  };

  template<typename T> inline
  typename std::enable_if<std::is_arithmetic<T>::value, void>::type
  visit(CoutWriter& writer, const T& value) {
    writer.out << value;
  }

  template<typename T> inline
  typename std::enable_if<std::is_enum<T>::value, void>::type
  visit(CoutWriter& writer, const T& value) {
    writer.out << (long long)(value);
  }

  inline void visit(CoutWriter& writer, const std::string& string) {
#if __cplusplus >= 201400L
    writer.out << std::quoted(string);
#else
    writer.out << '"' << string << '"';
#endif
  }
  
  template<typename Element>
  void visit(CoutWriter& writer, const std::vector<Element>& vector) {
    writer.out << '[';
    for (size_t i = 0; i < vector.size(); ++i) {
      if (i != 0) writer.out << ", ";
      visit(writer, vector[i]);
    }
    writer.out << ']';
  }

  template<>
  struct StructVisitor<CoutWriter> {
    const char* name;
    CoutWriter& writer;
    bool first;
    
    StructVisitor(const char* name_, CoutWriter& writer_)
      : name(name_), writer(writer_), first(true) {
      writer.out << name << '{';
    }

    ~StructVisitor() {
      writer.out << '}';
    }
    
    template<typename T>
    StructVisitor& field(const char* label, const T& value) {
      if (!first) writer.out << ", ";
      first = false;
      writer.out << label << ':';
      visit(writer, value);
      return *this;
    }
  };
}

/* Variable length integer encoding, used for binary serialization:
 *
 * Unsigned integers: each byte contains the lowest 7 bits of data and
 * 1 bit for "continue". If the continue bit is set, there's more
 * data. The last byte will have 0 in its continue bit. Examples:
 *
 * - 0b111 ==> 0:0000111
 * - 0b1111111100000000 ==> 1:0000000 1:1111110 0:0000011
 *
 * Signed integers: transform the number into an unsigned integer.
 *   - Positive integers X become X:0 (e.g. X << 1)
 *   - Negative integers X become (-X-1):1
 * Every signed integer has a unique unsigned representation this way.
 * No bits are wasted, and every signed integer can be represented.
 * NOTE: I believe this is equivalent to Google's ZigZag format
 * <https://developers.google.com/protocol-buffers/docs/encoding?hl=en#signed-integers>
 */

namespace traverse {
  
  inline void write_unsigned_int(std::stringbuf& out, uint64_t value) {
    do {
      uint8_t c = value & 0x7f;
      value >>= 7;
      if (value) c |= 0x80;
      out.sputc(c);
    } while(value);
  }

  inline bool read_unsigned_int(std::stringbuf& in, uint64_t& value) {
    uint64_t result = 0;
    for (int byte = 0; ; ++byte) {
      int c = in.sbumpc();
      if (c < 0) {
        return false;
      }
      bool endbit = (c & 0x80) == 0;
      result |= uint64_t(c & 0x7f) << (byte * 7);
      if (endbit) {
        value = result;
        return true;
      }
    }
  }

  inline void write_signed_int(std::stringbuf& out, int64_t value) {
    write_unsigned_int(out,
                       (value < 0)
                       ? (((-value-1) << 1) | 1)
                       : (value << 1));
  }

  inline bool read_signed_int(std::stringbuf& in, int64_t& value) {
    uint64_t decoded;
    bool status = read_unsigned_int(in, decoded);
    if (decoded & 1) {
      value = -(decoded >> 1)-1;
    } else {
      value = decoded >> 1;
    }
    return status;
  }
  
}

/* The binary serialize/deserialize uses a binary format and
 * stringbufs. Check deserializer.Errors() to see if anything went
 * wrong. It will be empty on success.
 */

namespace traverse {

  struct BinarySerialize {
    std::stringbuf out;
    BinarySerialize(): out(std::ios_base::out) {}
  };

  template<typename T> inline
  typename std::enable_if<std::is_arithmetic<T>::value && !std::is_signed<T>::value, void>::type
  visit(BinarySerialize& writer, const T& value) {
    uint64_t wide_value = uint64_t(value);
    write_unsigned_int(writer.out, wide_value);
  }

  template<typename T> inline
  typename std::enable_if<std::is_arithmetic<T>::value && std::is_signed<T>::value, void>::type
  visit(BinarySerialize& writer, const T& value) {
    write_signed_int(writer.out, value);
  }

  template <typename T>
  inline typename std::enable_if<std::is_enum<T>::value, void>::type
  visit(BinarySerialize& writer, const T& value) {
    visit(writer, typename std::underlying_type<T>::type(value));
  }
  
  inline void visit(BinarySerialize& writer, const std::string& string) {
    uint64_t size = string.size();
    write_unsigned_int(writer.out, size);
    writer.out.sputn(&string[0], size);
  }
  
  template<typename Element>
  void visit(BinarySerialize& writer, const std::vector<Element>& vector) {
    uint64_t size = vector.size();
    write_unsigned_int(writer.out, size);
    for (auto& element : vector) {
      visit(writer, element);
    }
  }


  struct BinaryDeserialize {
    std::stringbuf in;
    std::stringstream errors;
    BinaryDeserialize(const std::string& str): in(str, std::ios_base::in) {}
    std::string Errors() {
      if (in.in_avail() != 0) {
        errors << "Error: " << in.in_avail() << " extra bytes in message" << std::endl;
        in.pubseekoff(0, std::ios_base::end); // so we don't write error again
      }
      return errors.str();
    }
  };
 
  template<typename T> inline
  typename std::enable_if<std::is_arithmetic<T>::value && !std::is_signed<T>::value, void>::type
  visit(BinaryDeserialize& reader, T& value) {
    uint64_t wide_value;
    if (!read_unsigned_int(reader.in, wide_value)) {
      reader.errors << "Error: not enough data in buffer to read number" << std::endl;
    }
    value = T(wide_value);
  }
    
  template<typename T> inline
  typename std::enable_if<std::is_arithmetic<T>::value && std::is_signed<T>::value, void>::type
  visit(BinaryDeserialize& reader, T& value) {
    int64_t wide_value;
    if (!read_signed_int(reader.in, wide_value)) {
      reader.errors << "Error: not enough data in buffer to read number" << std::endl;
    }
    value = T(wide_value);
  }

  template<typename T> inline
  typename std::enable_if<std::is_enum<T>::value, void>::type
  visit(BinaryDeserialize& reader, T& value) {
    typename std::underlying_type<T>::type v;
    visit(reader, v);
    value = T(v);
  }

  inline void visit(BinaryDeserialize& reader, std::string& string) {
    uint64_t size = 0;
    if (!read_unsigned_int(reader.in, size)) {
      reader.errors << "Error: not enough data in buffer to read string size" << std::endl;
      return;
    }
    if (uint64_t(reader.in.in_avail()) < size) {
      reader.errors << "Error: not enough data in buffer to deserialize string" << std::endl;
      return;
    }
    string.resize(size);
    reader.in.sgetn(&string[0], size);
  }
  
  template<typename Element>
  void visit(BinaryDeserialize& reader, std::vector<Element>& vector) {
    uint64_t i = 0, size = 0;
    if (!read_unsigned_int(reader.in, size)) {
      reader.errors << "Error: not enough data in buffer to read vector size" << std::endl;
      return;
    }
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
