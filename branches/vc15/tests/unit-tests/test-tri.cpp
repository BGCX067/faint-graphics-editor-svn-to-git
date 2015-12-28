// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "geo/radii.hh"
#include "geo/rect.hh"
#include "geo/tri.hh"

static void verify_same(const faint::Point& p0, const faint::Point& p1){
  VERIFY(sloppy_equal(p0.x, p1.x));
  VERIFY(sloppy_equal(p0.y, p1.y));
}

static void verify_same(const faint::Radii& r0, const faint::Radii& r1){
  VERIFY(sloppy_equal(r0.x, r1.x));
  VERIFY(sloppy_equal(r0.y, r1.y));
}

void test_tri(){
  using namespace faint;

  {
    // <../doc/test-tri-a-1.png>
    Tri t({0,0},{10,0},{0,8});
    VERIFY(sloppy_equal(t.Width(), 10));
    VERIFY(sloppy_equal(t.Height(), 8));
    VERIFY(rather_zero(t.GetAngle()));
    VERIFY(sloppy_equal(t.Skew(), 0));
    VERIFY(t.Contains({0,0}));
    VERIFY(t.Contains({10,0}));
    VERIFY(t.Contains({0,8}));
    FWD(verify_same(bounding_rect(t).TopLeft(), {0,0}));
    FWD(verify_same(bounding_rect(t).BottomRight(), {10,8}));

    // Positions
    FWD(verify_same(center_point(t), {5.0,4.0}));
    FWD(verify_same(mid_P0_P1(t), {5.0, 0.0}));
    FWD(verify_same(mid_P0_P2(t), {0.0, 4.0}));
    FWD(verify_same(mid_P1_P3(t), {10.0, 4.0}));

    FWD(verify_same(get_radii(t), {5.0, 4.0}));
    VERIFY(sloppy_equal(area(t), 80));

    // Skew
    VERIFY(sloppy_equal(skewed(t, 10.0).Skew(), 10.0));
    VERIFY(sloppy_equal(area(skewed(t, 10.0)), area(t)));
  }

  {
    // <../doc/test-tri-b-1.png>
    Tri t({0,0}, {-10,0}, {0,8});
    VERIFY(sloppy_equal(t.Width(), 10));

    // For some (deliberate) reason, height is negative when flipped.
    KNOWN_ERROR(sloppy_equal(t.Height(), 8));

    VERIFY(t.GetAngle() == pi);
    VERIFY(sloppy_equal(t.Skew(), 0));
    VERIFY(sloppy_equal(area(t), 80));
    FWD(verify_same(bounding_rect(t).TopLeft(), {-10,0}));
    FWD(verify_same(bounding_rect(t).BottomRight(), {0,8}));

    // Positions
    FWD(verify_same(center_point(t), {-5, 4}));
    FWD(verify_same(mid_P0_P1(t), {-5, 0}));
    FWD(verify_same(mid_P0_P2(t), {0.0, 4.0}));
    FWD(verify_same(mid_P1_P3(t), {-10.0, 4.0}));

    // Skew
    VERIFY(sloppy_equal(skewed(t, 10.0).Skew(), 10.0));
    VERIFY(sloppy_equal(area(skewed(t, 10.0)), area(t)));
  }

  {
    // Lousy degenerate
    Tri t({0,0},{0,0},{0,0});
    VERIFY(sloppy_equal(t.Width(), 0));
    VERIFY(sloppy_equal(t.Height(), 0));
    VERIFY(rather_zero(t.GetAngle()));
    VERIFY(sloppy_equal(t.Skew(), 0));
    FWD(verify_same(mid_P0_P1(t), {0,0}));
    FWD(verify_same(mid_P0_P2(t), {0,0}));
    FWD(verify_same(mid_P1_P3(t), {0,0}));
  }

}
