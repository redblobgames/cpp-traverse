// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/**
 * Pattern matching helper function
 *
 * Rationale: I often want to run one of many pieces of code based on which
 * data variant I have. In ML-like languages this is done with pattern 
 * matching. In boost/mapbox variant this is one by creating a new class
 * inheriting static_visitor and defining an operator() for each variant.
 * That interface felt heavyweight to me so I made a lightweight interface.
 *
 * Usage:
 *
 *     variant<A, B, C> obj;
 *     match(obj,
 *           [&](const A& a) {
 *               ...
 *           },
 *           [&](const C& c) {
 *               ...
 *           }
 *         );
 *
 * The implementation does the simplest thing:
 *
 *   - If multiple clauses match, all of their functions run. This means
 *     'default' or 'else' clauses aren't possible. To change this,
 *     change run_if_match() to return a boolean and change match() to
 *     stop matching based on the return value. 
 *   - If no clauses match, no functions run, and there is no error/warning.
 *     To change this, change match() to _match() that returns a boolean,
 *     and then make the wrapper match() function check the return value.
 *     It should be possible to test this at compile time but it seems trickier.
 *   - Clauses can't return a value; instead I set a shared variable with
 *     any value that needs to be returned. To change this, make match()
 *     take a template parameter with the return type (defaulting to void), 
 *     and then have the lambdas return that type.
 *
 * Someone has a similar idea with a different implementation here:
 * <https://github.com/mapbox/variant/issues/113>
 */

#ifndef VARIANT_UTIL_H
#define VARIANT_UTIL_H

#include <type_traits>


namespace matchdetail {
  template<typename C, typename Arg>
  Arg lambda_argument(void(C::*)(const Arg&) const) {}

  template<typename Other>
  void lambda_argument(Other) {}

  template<typename Var, typename Fn>
  void run_if_match(const Var& var, const Fn& code) {
    typedef decltype(lambda_argument(&Fn::operator())) T;
    static_assert(!std::is_void<T>::value, "match() lambda must take const T&");
    if (var.template is<T>()) {
      T m = var.template get<T>();
      code(m);
    }
  }
}

template<typename Var>
void match(const Var&) {}

template<typename Var, typename First, typename ...Rest>
void match(const Var& var, First first, Rest... rest) {
  matchdetail::run_if_match(var, first);
  match(var, rest...);
}


#endif
