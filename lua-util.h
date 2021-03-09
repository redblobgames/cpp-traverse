// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/** 
 * These are some utility functions I wrote while working with the C-Lua API.
 */

#ifndef LUA_UTIL_H
#define LUA_UTIL_H

#include <lua.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>


/** 
 * Convert a lua expression into a value on the lua stack. 
 */
void lua_eval(lua_State* L, const std::string& expr) {
  std::string assignment_statement = "_ = " + expr;
  int error = luaL_loadstring(L, assignment_statement.c_str()) || lua_pcall(L, 0, 0, 0);
  if (error) {
    std::cerr << "lua_eval() error " << lua_tostring(L, -1);
    lua_pop(L, 1);
    lua_pushnil(L);
  } else {
    lua_getglobal(L, "_");
  }
}


/** 
 * Convert a value on the lua stack to a string representation
 * that looks as much as possible like lua source. This should
 * leave the lua stack as it found it. 
 */
std::string lua_repr(lua_State* L, int index=-1) {
  int type = lua_type(L, index);
  std::string output;
  switch (type) {
  case LUA_TNUMBER:
  case LUA_TSTRING:
    lua_pushvalue(L, index);       // stack: ... input
    lua_getglobal(L, "string");    // stack: ... input "string"
    lua_getfield(L, -1, "format"); // stack: ... input "string" string.format
    lua_remove(L, -2);             // stack: ... input string.format
    if (type == LUA_TNUMBER) {
      lua_pushliteral(L, "%g");    // stack: ... input string.format "%g"
    } else {
      lua_pushliteral(L, "%q");    // stack: ... input string.format "%q"
    }
    lua_pushvalue(L, -3);          // stack: ... input string.format "%q" input
    lua_call(L, 2, 1);             // stack: ... input output 
    output = std::string(lua_tostring(L, -1));
    lua_pop(L, 2);                // stack: ...
    return output;
  case LUA_TBOOLEAN:
    return lua_toboolean(L, index)? "true" : "false";
  case LUA_TNIL:
    return "nil";
  case LUA_TTABLE: {
    output = "{";

    // Print the array-like elements
    lua_len(L, -1);             // stack: ... length
    lua_Integer array_size = lua_tointeger(L, -1);
    lua_pop(L, 1);              // stack: ... 
    
    for (lua_Integer i = 1; i <= array_size; i++) {
      lua_rawgeti(L, -1, i);
      if (i > 1) { output += ", "; }
      output += lua_repr(L, -1);
      lua_pop(L, 1);
    }

    // Print the record-like elements, but sort them by key for reproducibility
    std::vector<std::string> key_value_pairs;
    lua_pushvalue(L, index);    // stack: ... input
    lua_pushnil(L);             // stack: ... input nil
    while (lua_next(L, -2)) {   // stack: ... input key value
      if (lua_type(L, -2) == LUA_TNUMBER) {
        lua_Integer key = lua_tointeger(L, -2);
        if (1 <= key && key <= array_size) {
          // Ignore this, as it's already been printed above
          lua_pop(L, 1);
          continue;
        }
      }

      std::string key = lua_repr(L, -2);
      bool shortened_key = false;
      if (lua_type(L, -2) == LUA_TSTRING) {
        // If it's a name (incomplete test), use the shorter syntax
        if (key.size() >= 3
            && key.front() == '"' && key.back() == '"'
            && std::isalpha(key[1])
            && std::all_of(key.begin()+1, key.end()-1, [](char c) { return std::isalnum(c); })) {
          key = key.substr(1, key.size()-2);
          shortened_key = true;
        }
      }

      std::string pair;
      if (!shortened_key) { pair += "["; }
      pair += key;
      if (!shortened_key) { pair += "]"; }
      pair += " = ";
      pair += lua_repr(L, -1);
      key_value_pairs.push_back(std::move(pair));
      
      lua_pop(L, 1);            // stack: ... input key
    }                           // stack: ... input

    std::sort(key_value_pairs.begin(), key_value_pairs.end());
    for (size_t i = 0; i < key_value_pairs.size(); i++) {
      if (array_size != 0 || i > 0) { output += ", "; }
      output += key_value_pairs[i];
    }
   
    output += "}";

    lua_pop(L, 1);              // stack: ...
    return output;
  }
    
  default:
    return lua_typename(L, type);
  }
}


/** 
 * Export the lua_repr() function into Lua. Use
 *
 *     lua_pushcfunction(L, export_lua_repr);
 *     lua_setglobal(L, "repr");
 */
int export_lua_repr(lua_State* L) {
  std::string s = lua_repr(L, 1);
  lua_pushlstring(L, s.data(), s.size());
  return 1;
}


#endif
