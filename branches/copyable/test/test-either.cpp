// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "bitmap/bitmap.hh"
#include "geo/intpoint.hh"
#include "util/color.hh"
#include "util/paint.hh"
#include "util/either.hh"

using namespace faint;
typedef Either<Bitmap, Color> either_t;

void test_either(){
  either_t either1(Color(255,0,255));
  Paint src = either1.Visit(
    [](const Bitmap&) -> Paint{
      FAIL();
    },
    [](const Color& c){
      return Paint(c);
    });
  ASSERT(src.IsColor());
  VERIFY(src.GetColor() == Color(255,0,255));

  either_t either2(either1);
  src = either2.Visit(
    [](const Bitmap&) -> Paint{
      FAIL();
    },
    [](const Color& c){
      return Paint(c);
    });
  ASSERT(src.IsColor());
  VERIFY(src.GetColor() == Color(255,0,255));
  VERIFY(either2.Get<Bitmap>().NotSet());
  VERIFY(either2.Get<Color>().IsSet());

  either_t either3(Bitmap(IntSize(10,10), Color(255,0,0)));
  src = either3.Visit(
    [](const Bitmap& bmp){
      return Paint(get_color(bmp, IntPoint(1,1)));
    },
    [](const Color&) -> Paint{
      FAIL();
    });
  ASSERT(src.IsColor());
  VERIFY(src.GetColor() == Color(255,0,0));

  either3.Set(Bitmap(IntSize(10,10), Color(0,0,255)));
  src = either3.Visit(
    [](const Bitmap& bmp){
      return Paint(get_color(bmp, IntPoint(1,1)));
    },
    [](const Color&) -> Paint{
      FAIL();
    });
  ASSERT(src.IsColor());
  VERIFY(src.GetColor() == Color(0,0,255));

  either3.Set(Color(0,255,0));
  src = either3.Visit(
    [](const Bitmap&) -> Paint{
      FAIL();
    },
    [](const Color& c){
      return Paint(c);
    });
  ASSERT(src.IsColor());
  VERIFY(src.GetColor() == Color(0,255,0));

  VERIFY(either3.Get<Bitmap>().NotSet());

  ASSERT(either3.Get<Color>().IsSet());
  VERIFY(either3.Get<Color>().Get() == Color(0,255,0));

}
