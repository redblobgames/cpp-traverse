// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include <lua.hpp>
#include "traverse.h"
#include "traverse-lua.h"

#include "test.h"
#include "lua-util.h"

/** Test LuaWriter can handle various data types */
void writer_unit_tests() {
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);

  traverse::LuaWriter writer{L};

  // Ints
  visit(writer, 5);
  TEST_EQ(lua_repr(L, -1), "5");
  lua_pop(L, 1);
  
  // Floats
  visit(writer, 0.5);
  TEST_EQ(lua_repr(L, -1), "0.5");
  lua_pop(L, 1);

  // Strings, including making sure \0 doesn't truncate it
  visit(writer, std::string("\0hello\xff", 7));
  TEST_EQ(lua_repr(L, -1), std::string("\"\\0hello\xff\"", 10));
  lua_pop(L, 1);

  // Structs, enums, enum classes, strings with quotes, vectors
  visit(writer, Polygon{BLUE, Mood::HULK_SMASH, "UFO\"1942\"", {{3, 5}, {4, 6}, {5, 7}}});
  TEST_EQ(lua_repr(L, -1), "{color = 1, mood = 2, name = \"UFO\\\"1942\\\"\", points = {{x = 3, y = 5}, {x = 4, y = 6}, {x = 5, y = 7}}}");
  lua_pop(L, 1);

  // Each test should leave the stack alone
  TEST_EQ(lua_gettop(L), 0);
  lua_close(L);
}


/** Test LuaReader can handle various data types */
void reader_unit_tests() {
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);

  traverse::LuaReader reader{L, std::cerr};

  // Ints
  int i;
  lua_eval(L, "5");
  visit(reader, i);
  TEST_EQ(i, 5);

  // Floats
  float f;
  lua_eval(L, "0.5");
  visit(reader, f);
  TEST_EQ(f, 0.5);

  // Strings
  std::string s;
  lua_eval(L, "\"hello\"");
  visit(reader, s);
  TEST_EQ(s, "hello");

  // Structs, enums, enum classes, strings with quotes, vectors
  Polygon p1;
  const Polygon p2 = {BLUE, Mood::HULK_SMASH, "UFO\"1942\"", {{3, 5}, {4, 6}, {5, 7}}};
  lua_eval(L, "{color = 1, mood = 2, name = \"UFO\\\"1942\\\"\", points = {{x = 3, y = 5}, {x = 4, y = 6}, {x = 5, y = 7}}}");
  visit(reader, p1);
  std::stringstream s1, s2;
  s1 << p1; s2 << p2;
  TEST_EQ(s1.str(), s2.str());

  // Each test should leave the stack alone
  TEST_EQ(lua_gettop(L), 0);
  lua_close(L);
}


/** Test that mismatched Lua/C++ data doesn't silently go through */
void mismatch_unit_tests() {
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);

  std::stringstream errors;
  traverse::LuaReader reader{L, errors};
  int i; std::string s; std::vector<int> v; Point p;

#define NO(LUA, VAR) { int top = lua_gettop(L); lua_eval(L, LUA); visit(reader, VAR); std::cerr << std::dec; if (errors.str().size() > 0) { std::cout << "   PASS " << __FILE__ << ':' << __LINE__ << ' ' << errors.str(); } else { std::cerr << " * FAIL " << __FILE__ << ':' << __LINE__ << std::endl; } errors.str(""); if (lua_gettop(L) != top) { std::cerr << " + FAIL stack not preserved" << std::endl; } }
  
  // Make sure ints and strings don't convert to each other
  NO("5", s);
  NO("\"5\"", i);

  // Catch inappropriate vector indices
  NO("{[-3] = 0}", v);
  NO("{[100] = 0}", v);
  NO("{a = 0}", v);
  NO("{[false] = 0}", v);

  // Catch extra lua fields
  NO("{x=3, y=4, z=5}", p);
  NO("{x=3, y=4, [1]=5}", p);

  // Catch missing lua fields
  NO("{x=3}", p);

  // Catch other type mismatches
  NO("1", v);
  NO("1", p);
  NO("\"hi\"", v);
  NO("\"hi\"", p);
  NO("{}", i);
  NO("{}", s);
  NO("print", i);
  NO("print", s);
  NO("print", v);
  NO("print", p);
  NO("false", i);
  NO("false", s);
  NO("false", v);
  NO("false", p);
  NO("nil", i);
  NO("nil", s);
  NO("nil", v);
  NO("nil", p);
#undef NO
  lua_close(L);
}


/** Test the ignore_* flags on LuaReader */
void ignore_flag_unit_tests() {
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);

  std::stringstream errors;
  traverse::LuaReader reader{L, errors};
  int i; std::string s; std::vector<int> v; Point p;

#define YES(LUA, VAR) lua_eval(L, LUA); visit(reader, VAR); std::cerr << std::dec; if (errors.str().size() == 0) { std::cout << "   PASS " << __FILE__ << ':' << __LINE__ << std::endl; } else { std::cerr << " * FAIL " << __FILE__ << ':' << __LINE__ << ' ' << errors.str(); } errors.str("")

  // Test silencing wrong types
  {
    reader.ignore_wrong_type = true;
    YES("5", s);
    YES("\"5\"", i);
    YES("1", v);
    YES("1", p);
    YES("\"hi\"", v);
    YES("\"hi\"", p);
    YES("{}", i);
    YES("{}", s);
    YES("print", i);
    YES("print", s);
    YES("print", v);
    YES("print", p);
    YES("false", i);
    YES("false", s);
    YES("false", v);
    YES("false", p);
    YES("nil", i);
    YES("nil", s);
    YES("nil", v);
    YES("nil", p);
    reader.ignore_wrong_type = false;
  }

  // Test silencing ignore_extra_field
  {
    reader.ignore_extra_field = true;
    YES("{[-3] = 0}", v);
    YES("{[100] = 0}", v);
    YES("{a = 0}", v);
    YES("{[false] = 0}", v);
    YES("{x=3, y=4, z=5}", p);
    YES("{x=3, y=4, [1]=5}", p);
    reader.ignore_extra_field = false;
  }

  // Test silencing ignore_missing_field
  {
    reader.ignore_missing_field = true;
    YES("{x=3}", p);
    reader.ignore_missing_field = false;
  }
  
  lua_close(L);
}


int main() {
  writer_unit_tests();
  reader_unit_tests();
  mismatch_unit_tests();
  ignore_flag_unit_tests();
}
