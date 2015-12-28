// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "geo/measure.hh"
#include "geo/pathpt.hh"

void test_bezier_distance(){
  using namespace faint;
  static const int subDivisions = 10;

  const coord lineLength = distance(Point(0,0), Point(10,10));
  VERIFY(14.142 < lineLength && lineLength < 14.1422);

  // Compare straight line with straight-bezier (coincident control
  // points
  const coord straightBezierLength = distance(Point(0,0),
    CubicBezier({10,10}, {5,5}, {5,5}), subDivisions);
  VERIFY(std::fabs(straightBezierLength - lineLength) < 0.01);

  // Verify that a curved bezier is longer than a straight bezier
  // between the same points.
  const coord curvedBezierLength = distance(Point(0,0),
    CubicBezier({10,10}, {10,0}, {10,0}), subDivisions);
  VERIFY(curvedBezierLength - straightBezierLength > 1);

  // Verify that the curved bezier is shorter than the manhattan
  // distance
  VERIFY(curvedBezierLength <
    distance(Point(0,0), Point(10,0)) +
    distance(Point(10, 0), Point(10,10)));
}
