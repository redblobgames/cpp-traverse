/*
 *
 * Copyright 2017 Red Blob Games <redblobgames@gmail.com>
 * License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>
 *
 * Generate binary serialization fuzz testing cases for AFL-fuzz
 *
 */

#include "traverse.h"
#include <iostream>
#include <fstream>
#include "test.h"

using namespace std::literals::string_literals;

Polygon P;
int FILENAME_COUNTER = 0;

void save() {
  std::ofstream f(std::string("/tmp/fuzz-input/") + std::to_string(FILENAME_COUNTER));
  ++FILENAME_COUNTER;
  traverse::BinarySerialize writer(*f.rdbuf());
  visit(writer, P);
}

int main() {
  P.color = BLUE; save();
  P.mood = Mood::SAD; save();
  P.name = "hello"s; save();
  P.points = {{3, 5}}; save();
  P.points = {{3, 5}, {-100, 900}}; save();
  P.points = {{1, 2}, {0x7fffffff, 1 << 31}}; save();
  P.points = {{1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}}; save();
  P.mood = Mood::HAPPY; save();
  P.name = "\0"s; save();
  P.name = "some \f u \n \n y c h \a \r \a c \t e \r s"s; save();
  P.name = "fuzz testing"; save();
  P.mood = Mood::HULK_SMASH; save();
  for (int i = 0; i < 50; i++) {
    P.points.emplace_back();
    P.points.back().x = i*10;
    P.points.back().y = i*17;
  } save();
}
