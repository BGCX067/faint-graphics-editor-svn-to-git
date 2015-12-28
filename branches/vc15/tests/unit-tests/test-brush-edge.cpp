// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "tests/test-util/text-bitmap.hh"
#include "bitmap/bitmap.hh"
#include "bitmap/brush.hh"
#include "geo/geo-func.hh"
#include "geo/line.hh"
#include "rendering/render-brush.hh"

using namespace faint;

void test_brush_edge(){
  {
    // Rectangle brush
    auto edge = brush_edge(rect_brush(2));
    VERIFY(edge.size() == 4); // Rectangle brush has 4 sides

    VERIFY(edge[0] == IntLineSegment({0,0}, {2,0})); // Top
    VERIFY(edge[1] == IntLineSegment({0,2}, {2,2})); // Bottom
    VERIFY(edge[2] == IntLineSegment({0,0}, {0,2})); // Left
    VERIFY(edge[3] == IntLineSegment({2,0}, {2,2})); // Right
  }

  {
    auto edge = brush_edge(create_brush(IntSize(4,4),
        // A circular brush
        " XX "
        "XXXX"
        "XXXX"
        " XX ",
        // Using 1 as alpha-value to ensure that low alpha values > 0
        // are considered "inside".
        {{' ', 0}, {'X', 1}}));
    VERIFY(edge.size() == 12);

    // Top edge
    VERIFY(edge[0] == IntLineSegment({1,0},{3,0}));

    // Steps from above
    VERIFY(edge[1] == IntLineSegment({0,1},{1,1}));
    VERIFY(edge[2] == IntLineSegment({3,1},{4,1}));

    // Steps from below
    VERIFY(edge[3] == IntLineSegment({0,3},{1,3}));
    VERIFY(edge[4] == IntLineSegment({3,3},{4,3}));

    // Bottom edge
    VERIFY(edge[5] == IntLineSegment({1,4},{3,4}));

    // Leftmost edge
    VERIFY(edge[6] == IntLineSegment({0,1},{0,3}));

    // Steps from the left
    VERIFY(edge[7] == IntLineSegment({1,0},{1,1}));
    VERIFY(edge[8] == IntLineSegment({1,3},{1,4}));

    // Steps from the right
    VERIFY(edge[9] == IntLineSegment({3,0},{3,1}));
    VERIFY(edge[10] == IntLineSegment({3,3},{3,4}));

    // Rightmost edge
    VERIFY(edge[11] == IntLineSegment({4,1},{4,3}));
  }
}
