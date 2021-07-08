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
   * All serializable fields must be public.
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


/* The CoutWriter just writes everything to std::cout */

namespace traverse {
  struct CoutWriter {
    std::ostream& out;
    CoutWriter(std::ostream& out_ = std::cout) : out(out_) {}
  };

  template<typename T> inline
  std::enable_if_t<std::is_arithmetic_v<T>>
  visit(CoutWriter& writer, const T& value) {
    writer.out << value;
  }

  template<typename T> inline
  std::enable_if_t<std::is_enum_v<T>>
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
  
  inline void write_unsigned_int(std::streambuf& out, uint64_t value) {
    do {
      uint8_t c = value & 0x7f;
      value >>= 7;
      if (value) c |= 0x80;
      out.sputc(c);
    } while(value);
  }

  inline bool read_unsigned_int(std::streambuf& in, uint64_t& value) {
    uint64_t result = 0;
    for (int byte = 0; ; ++byte) {
      int c = in.sbumpc();
      if (c < 0) {
        return false;
      }
      bool endbit = (c & 0x80) == 0;
      result |= uint64_t(c & 0x7f) << (byte * 7);
      if (endbit) {
        break;
      }
    }
    value = result;
    return true;
  }

  inline void write_signed_int(std::streambuf& out, int64_t value) {
    write_unsigned_int(out,
                       (value < 0)
                       ? ((uint64_t(-(value+1)) << 1) | 1)
                       : (value << 1));
  }

  inline bool read_signed_int(std::streambuf& in, int64_t& value) {
    uint64_t decoded = 0;
    bool status = read_unsigned_int(in, decoded);
    if (decoded & 1) {
      value = -int64_t(decoded >> 1)-1;
    } else {
      value = decoded >> 1;
    }
    return status;
  }
  
}

/* The binary serialize/deserialize uses a binary format and
 * streambufs. Check deserializer.Errors() to see if anything went
 * wrong. It will be empty on success.
 */

namespace traverse {

  struct BinarySerialize {
    std::streambuf& out;
    BinarySerialize(std::streambuf& out_): out(out_) {}
  };

  template<typename T> inline
  std::enable_if_t<std::is_arithmetic_v<T> && !std::is_signed_v<T>>
  visit(BinarySerialize& writer, const T& value) {
    uint64_t wide_value = uint64_t(value);
    write_unsigned_int(writer.out, wide_value);
  }

  template<typename T> inline
  std::enable_if_t<std::is_arithmetic_v<T> && std::is_signed_v<T>>
  visit(BinarySerialize& writer, const T& value) {
    write_signed_int(writer.out, value);
  }

  // Always treat char as unsigned
  inline void visit(BinarySerialize& writer, const char& value) {
    visit(writer, static_cast<unsigned char>(value));
  }
  inline void visit(BinarySerialize& writer, const signed char& value) {
    visit(writer, static_cast<unsigned char>(value));
  }

  template <typename T>
  inline std::enable_if_t<std::is_enum_v<T>>
  visit(BinarySerialize& writer, const T& value) {
    visit(writer, std::underlying_type_t<T>(value));
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
    std::streambuf& in;
    std::stringstream errors;
    BinaryDeserialize(std::streambuf& buf): in(buf) {}
    std::string Errors() { return errors.str(); }
  };
 
  template<typename T> inline
  std::enable_if_t<std::is_arithmetic_v<T> && !std::is_signed_v<T>>
  visit(BinaryDeserialize& reader, T& value) {
    uint64_t wide_value;
    if (!read_unsigned_int(reader.in, wide_value)) {
      reader.errors << "Error: not enough data in buffer to read number\n";
    }
    value = static_cast<T>(wide_value);
  }
    
  template<typename T> inline
  std::enable_if_t<std::is_arithmetic_v<T> && std::is_signed_v<T>>
  visit(BinaryDeserialize& reader, T& value) {
    int64_t wide_value;
    if (!read_signed_int(reader.in, wide_value)) {
      reader.errors << "Error: not enough data in buffer to read number\n";
    }
    value = static_cast<T>(wide_value);
  }

  // Always treat char as unsigned
  inline void visit(BinaryDeserialize& reader, char& value) {
    unsigned char u;
    visit(reader, u);
    value = static_cast<char>(u);
  }
  inline void visit(BinaryDeserialize& reader, signed char& value) {
    unsigned char u;
    visit(reader, u);
    value = static_cast<signed char>(u);
  }

  template<typename T> inline
  std::enable_if_t<std::is_enum_v<T>>
  visit(BinaryDeserialize& reader, T& value) {
    std::underlying_type_t<T> v;
    visit(reader, v);
    value = static_cast<T>(v);
  }

  inline void visit(BinaryDeserialize& reader, std::string& string) {
    uint64_t size = 0;
    if (!read_unsigned_int(reader.in, size)) {
      reader.errors << "Error: not enough data in buffer to read string size\n";
      return;
    }

    /* Copy blocks out of the input stream into the output string.
     * 
     * It would be simpler to resize the string to 'size' and read all
     * of it at once with sgetn(). However 'size' is untrusted input
     * and there may not actually be 'size' bytes available in the
     * stream.
     */
    const size_t buffersize = 1024;
    char buffer[buffersize];
    string.resize(0);
    size_t bytes_remaining = size;
    while (bytes_remaining > 0
           && reader.in.sgetc() != std::streambuf::traits_type::eof()) {
      size_t bytes_to_read = std::min(bytes_remaining, buffersize);
      size_t bytes_actually_read = reader.in.sgetn(buffer, bytes_to_read);
      string.append(buffer, buffer + bytes_actually_read);
      if (bytes_actually_read < bytes_to_read) {
        reader.errors << "Error: expected " << size
                      << " bytes in string but only found "
                      << (string.size() + bytes_actually_read) << "\n";
        return;
      }
      bytes_remaining -= bytes_actually_read;
    }
  }
  
  template<typename Element>
  void visit(BinaryDeserialize& reader, std::vector<Element>& vector) {
    uint64_t i = 0, size = 0;
    if (!read_unsigned_int(reader.in, size)) {
      reader.errors << "Error: not enough data in buffer to read vector size\n";
      return;
    }
    vector.clear();
    for (; i < size && reader.in.sgetc() != std::streambuf::traits_type::eof(); ++i) {
      vector.emplace_back();
      visit(reader, vector.back());
    }
    if (i != size) {
      reader.errors << "Error: expected " << size
                    << " elements in vector but only found "
                    << i << "\n";
    }
  }
}


#endif
