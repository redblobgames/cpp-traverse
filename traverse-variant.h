// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/**
 * Plug-in to make mapbox::variant work with the traverse library.
 * 
 * The traverse library is "multimethod" style so it can be extended with
 * both new nouns (data types to be visited) and verbs (visit operations).
 * This file only defines variant + cout and variant + binary serialization,
 * but does not define variant + json or variant + lua.
 */

#ifndef TRAVERSE_VARIANT_H
#define TRAVERSE_VARIANT_H

#include "traverse.h"
#include "variant/variant.hpp"
#include "variant/variant_io.hpp"


namespace traverse {
  // Using mapbox's variant here but if you use boost or another variant,
  // change the includes above and these typedefs:
  template<typename ...T> using variant = mapbox::util::variant<T...>;
  using mapbox::util::apply_visitor;

  
  struct CoutVariantVisitor {
    CoutWriter& writer;
    template<typename T> void operator()(const T& value) {
      visit(writer, value);
    }
  };
  
  template<typename ...Variants>
  void visit(CoutWriter& writer, const variant<Variants...>& value) {
      apply_visitor(CoutVariantVisitor{writer}, value);
  }

  
  struct BinarySerializeVariantHelper {
    BinarySerialize& writer;
    template<typename T> void operator()(const T& value) {
      visit(writer, value);
    }
  };
  
  template<typename ...Variants>
  void visit(BinarySerialize& writer, const variant<Variants...>& value) {
    unsigned which = value.which();
    visit(writer, which);
    apply_visitor(BinarySerializeVariantHelper{writer}, value);
  }


  template<typename VariantType>
  void deserialize_variant_helper(BinaryDeserialize& reader,
                                  unsigned which, unsigned index,
                                  VariantType&) {
    reader.errors << "Error: tried to deserialize variant " << which
                  << " but there were only " << index << " types."
                  << std::endl;
  }
  
  template<typename VariantType, typename First, typename ...Rest>
  void deserialize_variant_helper(BinaryDeserialize& reader,
                                  unsigned which, unsigned index,
                                  VariantType& value) {
    if (which == index) {
      value.template set<First>();
      visit(reader, value.template get<First>());
    } else {
      deserialize_variant_helper<VariantType, Rest...>(reader, which, index+1, value);
    }
  }

  template<typename ...Variants>
  void visit(BinaryDeserialize& reader, variant<Variants...>& value) {
    unsigned which;
    visit(reader, which);
    deserialize_variant_helper<variant<Variants...>, Variants...>
      (reader, which, 0, value);
  }
  
}



#endif
