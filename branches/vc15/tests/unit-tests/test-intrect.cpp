// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "geo/int-point.hh"
#include "geo/int-size.hh"
#include "geo/int-rect.hh"

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

  // IntSize constructor
  VERIFY(IntRect(IntPoint(0,0), IntSize(2,2)) ==
    IntRect(IntPoint(0,0), IntPoint(1,1)));

  // smallest, largest
  IntRect area1(IntPoint(0,0), IntSize(1,1));
  IntRect area4(IntPoint(0,0), IntSize(2,2));
  IntRect area2A(IntPoint(0,0), IntSize(2,1));
  IntRect area2B(IntPoint(0,0), IntSize(1,2));
  VERIFY(smallest(area1, area4) == area1);
  VERIFY(largest(area1, area4) == area4);
  VERIFY(largest(area2A, area2B) == area2B);
  VERIFY(smallest(area2A, area2B) == area2A);
}
