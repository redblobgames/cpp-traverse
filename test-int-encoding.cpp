// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

// Test that integers encode and decode back to the same integer. This
// program is a bit verbose but it's useful to see how unsigned and
// signed integers are represented.

#include "traverse.h"
#include <iostream>
#include "test.h"


void show_u(uint64_t x) {
  std::stringbuf out;
  traverse::write_unsigned_int(out, x);
  std::cout << "Encode-U " << std::hex << x << " as [";
  for (auto c : out.str()) {
    for (int bit = 7; bit >= 0; --bit) {
      std::cout << ((c >> bit) & 1);
    }
    std::cout << "  ";
  }
  std::cout << "]" << std::endl;
}

void show_s(int64_t x) {
  std::stringbuf out;
  traverse::write_signed_int(out, x);
  std::cout << "Encode-S " << std::hex << x << " as [";
  for (auto c : out.str()) {
    for (int bit = 7; bit >= 0; --bit) {
      std::cout << ((c >> bit) & 1);
    }
    std::cout << "  ";
  }
  std::cout << "]" << std::endl;
}

void test_roundtrip_u(uint64_t x, bool quiet=false) {
  std::stringbuf msg;
  traverse::write_unsigned_int(msg, x);
  
  uint64_t y;
  if (traverse::read_unsigned_int(msg, y)) {
    if (quiet && x == y) return;
  }
  show_u(x);
  TEST_EQ(x, y);
}

void test_roundtrip_s(int64_t x, bool quiet=false) {
  std::stringbuf msg;
  traverse::write_signed_int(msg, x);

  int64_t y;
  if (traverse::read_signed_int(msg, y)) {
    if (quiet && x == y) return;
  }
  show_s(x);
  TEST_EQ(x, y);
}


int main() {
  test_roundtrip_u(0);
  test_roundtrip_u(1);
  test_roundtrip_u(0x7ffffffffffffffe);
  test_roundtrip_u(0x7fffffffffffffff);
  test_roundtrip_u(0x8000000000000000);
  test_roundtrip_u(0x8000000000000001);
  test_roundtrip_u(0xfffffffffffffffe);
  test_roundtrip_u(0xffffffffffffffff);
  for (uint64_t x = 1; x < 0x7fffffffffffffff; x *= 2) {
    test_roundtrip_u(x-1, true);
    test_roundtrip_u(x, true);
    test_roundtrip_u(x+1, true);
  }
  for (uint64_t x = 0; x < 1000000; x++) {
    test_roundtrip_u(x, true);
  }

  test_roundtrip_s(0);
  test_roundtrip_s(1);
  test_roundtrip_s(-2);
  test_roundtrip_s(-1);
  test_roundtrip_s(0x7ffffffffffffffe);
  test_roundtrip_s(0x7fffffffffffffff);
  test_roundtrip_s(0x8000000000000000);
  test_roundtrip_s(0x8000000000000001);
  for (int64_t x = 1; x < 0x3fffffffffffffff; x *= 2) {
    test_roundtrip_s(-x-1, true);
    test_roundtrip_s(-x, true);
    test_roundtrip_s(-x+1, true);
    test_roundtrip_s(x-1, true);
    test_roundtrip_s(x, true);
    test_roundtrip_s(x+1, true);
  }
  for (int64_t x = -1000000; x < 1000000; x++) {
    test_roundtrip_s(x, true);
  }
}
