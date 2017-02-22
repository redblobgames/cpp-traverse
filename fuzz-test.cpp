/*
 *
 * Copyright 2017 Red Blob Games <redblobgames@gmail.com>
 * License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>
 *
 * Binary deserialize one struct from stdin, for fuzz testing
 *
 */

#include "traverse.h"
#include <iostream>
#include <fstream>
#include "test.h"

using namespace std;

int main() {
  Polygon P;
  traverse::BinaryDeserialize reader(*cin.rdbuf());
  visit(reader, P);
  std::cout << P << "\n";
  std::cout << reader.Errors() << "\n";
}
