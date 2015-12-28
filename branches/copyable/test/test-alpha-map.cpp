// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "bitmap/alpha-map.hh"
#include "bitmap/bitmap.hh"
#include "bitmap/brush.hh"
#include "geo/intpoint.hh"
#include "geo/intsize.hh"
#include "geo/intrect.hh"
#include "geo/offsat.hh"

using namespace faint;
namespace bmp_impl{
bool intersect(const Bitmap&, const Offsat<AlphaMapRef>&);
}

void test_alpha_map(){
  Bitmap bmp(IntSize(10,5));
  AlphaMap map(IntSize(10,5));
  static_assert(!std::is_copy_constructible<AlphaMap>::value,
    "AlphaMap should not be copy constructible.");

  VERIFY(bmp_impl::intersect(bmp, offsat(map.FullReference(), IntPoint(0,0))));

  // Same size AlphaMapRef offset tests
  auto fullRef = map.FullReference();
  VERIFY(fullRef.GetSize() == IntSize(10,5));
  VERIFY(bmp_impl::intersect(bmp, offsat(fullRef, 10,0)));
  VERIFY(bmp_impl::intersect(bmp, offsat(fullRef, -10,0)));
  VERIFY(bmp_impl::intersect(bmp, offsat(fullRef, 0,5)));
  VERIFY(bmp_impl::intersect(bmp, offsat(fullRef, 0,-5)));

  VERIFY(!bmp_impl::intersect(bmp, offsat(fullRef, -11,0)));
  VERIFY(!bmp_impl::intersect(bmp, offsat(fullRef, 11,0)));
  VERIFY(!bmp_impl::intersect(bmp, offsat(fullRef, 0,-6)));
  VERIFY(!bmp_impl::intersect(bmp, offsat(fullRef, 0,6)));

  // Smaller AlphaMapRef offset tests
  auto ref = map.SubReference(IntRect(IntPoint(1,2), IntPoint(3,3)));
  VERIFY(ref.GetSize() == IntSize(3,2));
  VERIFY(bmp_impl::intersect(bmp, offsat(ref, 0,0)));
  VERIFY(bmp_impl::intersect(bmp, offsat(ref, 10,5)));
  VERIFY(!bmp_impl::intersect(bmp, offsat(ref, 10,6)));
  VERIFY(!bmp_impl::intersect(bmp, offsat(ref, 11,5)));

  VERIFY(bmp_impl::intersect(bmp, offsat(ref, -3,0)));
  VERIFY(!bmp_impl::intersect(bmp, offsat(ref, -4,0)));

  VERIFY(bmp_impl::intersect(bmp, offsat(ref, 0,-2)));
  VERIFY(!bmp_impl::intersect(bmp, offsat(ref, 0,-3)));

  VERIFY(bmp_impl::intersect(bmp, offsat(ref, 10,0)));
  VERIFY(!bmp_impl::intersect(bmp, offsat(ref, 11,0)));

  VERIFY(bmp_impl::intersect(bmp, offsat(ref, 0,5)));
  VERIFY(!bmp_impl::intersect(bmp, offsat(ref, 0,6)));

  // Adding values
  map.Add(0,0, 10);
  VERIFY(map.Get(0,0) == 10);
  map.Add(0,0, 10);
  VERIFY(map.Get(0,0) == 20);
  map.Add(0,0, 235);
  VERIFY(map.Get(0,0) == 255);
  map.Add(0,0,1);
  VERIFY(map.Get(0,0) == 255);

  // Setting
  map.Set(0,0,100);
  VERIFY(map.Get(0,0) == 100);

  map.Set(9,4,20);
  VERIFY(map.Get(9,4) == 20);

  // Verify accessing values via reference
  map.Set(1,2, 10);
  VERIFY(ref.Get(0,0) == 10);

  map.Set(3,2, 11);
  VERIFY(ref.Get(2,0) == 11);

  map.Set(3,3, 12);
  VERIFY(ref.Get(2,1) == 12);

  // Valid content in copy
  auto copy = map.SubCopy(IntRect(IntPoint(1,2), IntPoint(3,3)));
  auto copyRef = copy.FullReference();
  VERIFY(copyRef.Get(0,1) == 0);
  VERIFY(copyRef.Get(2,1) == 12);

  // Copy does not affect source
  copy.Set(0,0,103);
  VERIFY(copy.Get(0,0) == 103);
  VERIFY(map.Get(0,0) != 103);

  // Single pixel brush
  Brush b1({1,1});
  b1.Set(0,0, 255);
  stroke(map,{1,1},{8,1}, b1);
  VERIFY(map.Get(0,1) != 255);

  VERIFY(map.Get(1,1) == 255);
  VERIFY(map.Get(1,2) != 255);

  VERIFY(map.Get(8,0) != 255);
  VERIFY(map.Get(8,1) == 255);
  VERIFY(map.Get(8,2) != 255);
  VERIFY(map.Get(9,1) != 255);

  // Reset
  map.Reset(IntSize(10,5));
  VERIFY(map.Get(1,1) == 0);

  // Two pixel tall brush
  Brush b2({1,2});
  b2.Set(0,0,255);
  b2.Set(0,1,255);
  stroke(map,{1,2},{8,2}, b2);
  VERIFY(map.Get(1,0) != 255);
  VERIFY(map.Get(1,1) == 255); // Apparently two pixel tall grows upwards
  VERIFY(map.Get(1,2) == 255);
  VERIFY(map.Get(1,3) != 255);

  VERIFY(map.Get(8,0) != 255);
  VERIFY(map.Get(8,1) == 255);
  VERIFY(map.Get(8,2) == 255);
  VERIFY(map.Get(8,3) != 255);

  VERIFY(map.Get(4,0) != 255);
  VERIFY(map.Get(4,1) == 255);
  VERIFY(map.Get(4,2) == 255);
  VERIFY(map.Get(4,3) != 255);

  VERIFY(map.Get(0,0) != 255);
  VERIFY(map.Get(0,1) != 255);
  VERIFY(map.Get(0,2) != 255);
  VERIFY(map.Get(0,3) != 255);

  VERIFY(map.Get(9,0) != 255);
  VERIFY(map.Get(9,1) != 255);
  VERIFY(map.Get(9,2) != 255);
  VERIFY(map.Get(9,3) != 255);
}
