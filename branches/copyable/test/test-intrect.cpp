// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "geo/intpoint.hh"
#include "geo/intrect.hh"

void test_intrect(){
  using namespace faint;
  IntRect r(IntPoint(-1,-2), IntPoint(10,20));
  VERIFY(r.TopLeft() == IntPoint(-1,-2));
  VERIFY(r.BottomRight() == IntPoint(10,20));
  VERIFY(r.Left() == -1);
  VERIFY(r.Right() == 10);
  VERIFY(r.Top() == -2);
  VERIFY(r.Bottom() == 20);
  VERIFY(r.w == 12);
  VERIFY(r.h == 23);
  VERIFY(!empty(r));
  VERIFY(area(r) == 12 * 23);
  VERIFY(r.Contains(IntPoint(-1,-2)));
  VERIFY(r.Contains(IntPoint(10,20)));
  VERIFY(!r.Contains(IntPoint(11,20)));
  VERIFY(!r.Contains(IntPoint(-2,-2)));

  IntRect r2(IntPoint(100,100), IntPoint(120,110));
  VERIFY(empty(intersection(r, r2)));

  auto u = union_of(r,r2);
  VERIFY(!empty(u));
  VERIFY(u.TopLeft() == IntPoint(-1,-2));
  VERIFY(u.BottomRight() == IntPoint(120,110));
}
