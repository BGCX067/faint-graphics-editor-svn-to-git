// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "tests/test-util/text-bitmap.hh"
#include "bitmap/brush.hh"
#include "rendering/render-brush.hh"

void test_brush_overlay(){
  using namespace faint;
  AlphaMap map({1,1});

  init_brush_overlay(map, create_brush(IntSize(4,4),
      // A circular brush
      " XX "
      "XXXX"
      "XXXX"
      " XX ",
      {{' ', 0}, {'X', 1}}));

  check(map,
    " XX "
    "XXXX"
    "XXXX"
    " XX ",
    {{' ', 0}, {'X', 1}});
  VERIFY(map.GetSize() == IntSize(4,4));

  // Set the same brush again to ensure the map is reset
  init_brush_overlay(map, create_brush(IntSize(4,4),
      // A circular brush
      " XX "
      "XXXX"
      "XXXX"
      " XX ",
      {{' ', 0}, {'X', 1}}));

  check(map,
    " XX "
    "XXXX"
    "XXXX"
    " XX ",
    {{' ', 0}, {'X', 1}});
  VERIFY(map.GetSize() == IntSize(4,4));

  // Set a brush with different dimensions
  init_brush_overlay(map, create_brush(IntSize(2,6),
      // A circular brush
      "XO"
      "O "
      "XO"
      "OX"
      " O"
      "OX",
      {{' ', 0}, {'X', 1}, {'O', 255}}));
  check(map,
      "XO"
      "O "
      "XO"
      "OX"
      " O"
      "OX",
    {{' ', 0}, {'X', 1}, {'O', 255}});
  VERIFY(map.GetSize() == IntSize(2,6));
}
