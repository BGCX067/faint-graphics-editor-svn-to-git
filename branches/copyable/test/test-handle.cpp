// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "geo/handle-func.hh"

void test_handle(){
  using namespace faint;
  const Point TL(0,0);
  const Point TR(10,0);
  const Point BL(0,10);
  const Point BR(10,10);

  Tri tri = {TL, TR, BL};
  VERIFY(get<Handle::P0>(tri) == TL);
  VERIFY(get<Handle::P1>(tri) == TR);
  VERIFY(get<Handle::P2>(tri) == BL);
  VERIFY(get<Handle::P3>(tri) == BR);
  VERIFY(get<Handle::P0P1>(tri) == LineSegment(TL, TR));
  VERIFY(get<Handle::P0P2>(tri) == LineSegment(TL, BL));
  VERIFY(get<Handle::P2P3>(tri) == LineSegment(BL, BR));
  VERIFY(get<Handle::P1P3>(tri) == LineSegment(TR, BR));

  Rect rect = {TL, BR};
  VERIFY(get<Handle::P0>(rect) == TL);
  VERIFY(get<Handle::P1>(rect) == TR);
  VERIFY(get<Handle::P2>(rect) == BL);
  VERIFY(get<Handle::P3>(rect) == BR);
  VERIFY(get<Handle::P0P1>(rect) == LineSegment(TL, TR));
  VERIFY(get<Handle::P0P2>(rect) == LineSegment(TL, BL));
  VERIFY(get<Handle::P2P3>(rect) == LineSegment(BL, BR));
  VERIFY(get<Handle::P1P3>(rect) == LineSegment(TR, BR));
}
