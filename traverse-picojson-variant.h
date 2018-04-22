// Copyright 2018 Red Blob Games <redblobgames@gmail.com>
// https://github.com/redblobgames/cpp-traverse
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/**
 * traverse-variant.h makes mapbox::variant work with traverse's
 * built in formats (cout, binary serialization).
 *
 * traverse-picojson.h makes traverse work with picojson, but only
 * with built-in traverse data types (int, string, array, object).
 *
 * traverse-picojson-variant.h fills in the gap: picojson + variant
 * by writing variants as an object {variant: ___, data: ___}.
 */

#ifndef TRAVERSE_PICOJSON_VARIANT_H
#define TRAVERSE_PICOJSON_VARIANT_H

#include "traverse.h"
#include "traverse-picojson.h"
#include "traverse-variant.h"

namespace traverse {
  // Using mapbox's variant here but if you use boost or another variant,
  // change the includes above and these typedefs:
  template<typename ...T> using variant = mapbox::util::variant<T...>;
  using mapbox::util::apply_visitor;

  struct PicoJsonWriterVariantHelper {
    PicoJsonWriter& writer;
    template<typename T> void operator()(const T& value) {
      visit(writer, value);
    }
  };
  
  template<typename ...Variants>
  void visit(PicoJsonWriter& writer, const variant<Variants...>& value) {
    picojson::value::object output;
    picojson::value output_which;
    picojson::value output_data;
    PicoJsonWriter writer_which = {output_which};
    PicoJsonWriter writer_data = {output_data};
    unsigned which = value.which();
    visit(writer_which, which);
    apply_visitor(PicoJsonWriterVariantHelper{writer_data}, value);
    output["which"] = output_which;
    output["data"] = output_data;
    writer.out = picojson::value(output);
  }


  template<typename VariantType>
  void deserialize_variant_helper(PicoJsonReader& reader,
                                  unsigned which, unsigned index,
                                  VariantType&) {
    reader.errors << "Error: tried to read variant " << which
                  << " but there were only " << index << " types."
                  << std::endl;
  }
  
  template<typename VariantType, typename First, typename ...Rest>
  void deserialize_variant_helper(PicoJsonReader& reader,
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
  void visit(PicoJsonReader& reader, variant<Variants...>& value) {
    auto input = reader.in.get<picojson::value::object>();
    auto input_which = input.find("which");
    if (input_which == input.end()) {
      reader.errors << "Error: JSON object missing field 'which'\n";
      return;
    }
    auto input_data = input.find("data");
    if (input_data == input.end()) {
      reader.errors << "Error: JSON object missing field 'data'\n";
      return;
    }

    // TODO: check that there are no other fields
    
    PicoJsonReader reader_which{input_which->second, reader.errors};
    unsigned which = 0;
    visit(reader_which, which);
    
    PicoJsonReader reader_data{input_data->second, reader.errors};
    deserialize_variant_helper<variant<Variants...>, Variants...>
      (reader_data, which, 0, value);
  }
}


#endif
