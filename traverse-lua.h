// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/**
 * Traverse-Lua extension for Lua 5.2.
 *
 * Example usage for C++ to Lua:
 *
 *     lua_State* L;
 *     traverse::LuaWriter luawriter{L};
 *     visit(luawriter, yourobject);
 *     // this leaves the object at the top of the lua stack
 * 
 * Example usage for Lua to C++:
 *
 *     // there should be a lua object at the top of the stack
 *     std::stringstream errors;
 *     traverse::LuaReader luareader{L, errors};
 *     visit(luareader, yourobject);
 *     if (!errors.empty()) { throw "read error"; }
 *     // the value will be popped off the lua stack
 *
 */


#ifndef TRAVERSE_LUA_H
#define TRAVERSE_LUA_H

#include "traverse.h"
#include "lua.hpp"

namespace traverse {

  /** 
   * The LuaWriter will take a value from C++ and push a Lua
   * equivalent onto the Lua stack. Ints and floats both become Lua
   * numbers. Vectors and structs both become Lua tables.
   */
  struct LuaWriter {
    lua_State* L;
  };
  
  template<typename T> inline
  typename std::enable_if<is_enum_or_number<T>::value, void>::type
  visit(LuaWriter& writer, const T& value) {
    lua_pushnumber(writer.L, double(value));
  }

  inline void visit(LuaWriter& writer, const std::string& string) {
    lua_pushlstring(writer.L, string.data(), string.size());
  }

  template<typename Element>
  void visit(LuaWriter& writer, const std::vector<Element>& vector) {
    lua_createtable(writer.L, vector.size(), 0);
    for (size_t i = 0; i != vector.size(); i++) {
      visit(writer, vector[i]);
      lua_rawseti(writer.L, -2, i+1);
    }
  }
  
  template<>
  struct StructVisitor<LuaWriter> {
    const char* name;
    LuaWriter& writer;
    
    StructVisitor(const char* name_, LuaWriter& writer_)
      : name(name_), writer(writer_) {
      lua_newtable(writer.L);
    }
    
    template<typename T>
    StructVisitor& field(const char* label, const T& value) {
      lua_pushstring(writer.L, label);
      visit(writer, value);
      lua_rawset(writer.L, -3);
      return *this;
    }
  };


  /** 
   * The LuaReader will convert the value at the top of the Lua stack
   * to a C++ value, and pop it off the Lua stack. Use lua_pushvalue
   * if you need to keep a copy or if you need to convert a value elsewhere
   * on the stack.
   *
   * The reader only checks the structural validity of the data structure; the 
   * caller must check the semantic validity of the data, e.g.:
   *
   * - Numbers are in range
   * - Enums are valid
   * - Strings are valid
   * - Vectors have the right length
   * - Contents of structs and vectors are valid
   * 
   * If reading untrusted data, write a validation function that runs
   * after the LuaReader produces a value.
   * 
   * The reader may not be able to convert some Lua data into C++ data.
   * Errors will be written to the 'errors' stream. Check that stream 
   * before using the value written by the traversal. To ignore a class
   * of errors, set the corresponding ignore_ field to true before visit().
   *
   * - ignore_wrong_type: in case of a type mismatch, leave the value unchanged.
   * - ignore_missing_field: if the C++ struct has a field not found in the
   *      Lua table, leave the value unchanged.
   * - ignore_extra_field: if the Lua table has a field not found on the C++
   *      struct, or if the Lua table has a negative or non-contiguous index
   *      when converting to a C++ vector, or if the Lua table has a non-numeric
   *      index when converting to a C++ vector, ignore that field/entry.
   */
  struct LuaReader {
    lua_State* L;
    std::ostream& errors;
    
    bool ignore_wrong_type = false;
    bool ignore_missing_field = false;
    bool ignore_extra_field = false;
  };

  template<typename T> inline
  typename std::enable_if<is_enum_or_number<T>::value, void>::type
  visit(LuaReader& reader, T& value) {
    if (lua_type(reader.L, -1) != LUA_TNUMBER) {
      if (!reader.ignore_wrong_type) {
        reader.errors << "Error: expected Lua number; skipping" << std::endl;
      }
      lua_pop(reader.L, 1);
      return;
    }

    if (std::is_integral<T>::value || std::is_enum<T>::value) {
      value = T(lua_tointeger(reader.L, -1));
    } else {
      value = T(lua_tonumber(reader.L, -1));
    }
    lua_pop(reader.L, 1);
  }

  inline void visit(LuaReader& reader, std::string& string) {
    if (lua_type(reader.L, -1) != LUA_TSTRING) {
      if (!reader.ignore_wrong_type) {
        reader.errors << "Error: expected Lua string; skipping" << std::endl;
      }
      lua_pop(reader.L, 1);
      return;
    }

    size_t size;
    const char* data = lua_tolstring(reader.L, -1, &size);
    string = std::string(data, size);
    lua_pop(reader.L, 1);
  }

  template<typename Element>
  void visit(LuaReader& reader, std::vector<Element>& vector) {
    if (!lua_istable(reader.L, -1)) {
      if (!reader.ignore_wrong_type) {
        reader.errors << "Error: expected Lua array(table); skipping" << std::endl;
      }
      lua_pop(reader.L, 1);
      return;
    }

    size_t size;                // stack: ... input
    lua_len(reader.L, -1);      // stack: ... input size
    visit(reader, size);        // stack: ... input
    
    vector.resize(size);
    for (size_t i = 0; i < size; i++) {
      lua_rawgeti(reader.L, -1, i+1); // stack: ... input input[i+1]
      visit(reader, vector[i]);       // stack: ... input
    }

    // Iterate through the table and make sure it doesn't have anything else
    lua_pushnil(reader.L);      // stack: ... input nil
    while (lua_next(reader.L, -2)) { // stack: ... input key value
      if (lua_type(reader.L, -2) != LUA_TNUMBER) {
        if (!reader.ignore_extra_field) {
          reader.errors << "Error: converting Lua table to std::vector, found non-numeric key"
                        << std::endl;
        }
      } else {
        lua_Number index = lua_tointeger(reader.L, -2);
        if (index < 1 || index > size) {
          if (!reader.ignore_extra_field) {
            reader.errors << "Error: converting Lua table size=" << size
                          << " to std::vector, found key=" << index
                          << std::endl;
          }
        }
      }
      
      lua_pop(reader.L, 1);     // stack: ... input key 
    }                           // stack: ... input

    // Remove input from stack
    lua_pop(reader.L, 1);
  }

  template<>
  struct StructVisitor<LuaReader> {
    const char* name;
    LuaReader& reader;
    std::vector<std::string> lua_field_names;
    bool is_table;
    
    StructVisitor(const char* name_, LuaReader& reader_)
      : name(name_), reader(reader_) {
      if (!lua_istable(reader.L, -1)) {
        if (!reader.ignore_wrong_type) {
          reader.errors << "Error: expected Lua object(table) to read into struct "
                        << name << "; skipping" << std::endl;
        }
        is_table = false;
        return;
      }
      is_table = true;
      
      // Iterate through the table to find all the string keys; this
      // is used for generating warnings about fields that weren't
      // transferred to the C++ side
      lua_pushnil(reader.L);    // stack: ... input nil
      while (lua_next(reader.L, -2)) { // stack: ... input key value
        if (lua_type(reader.L, -2) != LUA_TSTRING) {
          if (!reader.ignore_extra_field) {
            reader.errors << "Error: converting Lua table to " << name
                          << ", found non-string key"
                          << std::endl;
          }
        } else {
          size_t size = 0;
          const char* field = lua_tolstring(reader.L, -2, &size);
          lua_field_names.push_back(std::string(field, size));
        }
        lua_pop(reader.L, 1);   // stack: ... input key
      }                         // stack: ... input
    }

    ~StructVisitor() {
      lua_pop(reader.L, 1);     // stack: ...

      if (!reader.ignore_extra_field && !lua_field_names.empty()) {
        reader.errors << "Error: Lua object contains extra keys: ";
        for (auto field : lua_field_names) {
          reader.errors << field << ' ';
        }
        reader.errors << std::endl;
      }
    }
    
    template<typename T>
    StructVisitor& field(const char* label, T& value) {
      if (!is_table) { return *this; }
      
      lua_getfield(reader.L, -1, label); // stack: ... input input[label]
      if (lua_isnil(reader.L, -1)) {
        if (!reader.ignore_missing_field) {
          reader.errors << "Error: Lua object missing field " << label << std::endl;
        }
        lua_pop(reader.L, 1);   // stack: ... input
      } else {
        if (!reader.ignore_extra_field) {
          // To detect this error, we need to track which fields were used
          auto it = find(lua_field_names.begin(), lua_field_names.end(), label);
          if (it == lua_field_names.end()) {
            reader.errors << "Error: lua table lost field during traverse" << std::endl;
          } else {
            std::swap(*it, lua_field_names.back());
            lua_field_names.pop_back();
          }
        }
        visit(reader, value);   // stack: ... input
      }
      return *this;
    }
  };
  
}


#endif
