// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "geo/int-point.hh"

void test_intpoint(){
  using namespace faint;
  IntPoint p0;
  VERIFY(p0 == IntPoint(0,0));
  VERIFY(fully_positive(p0));
  VERIFY(max_coords(IntPoint(-1,-2), IntPoint(0,0)) == IntPoint(0,0));
}
