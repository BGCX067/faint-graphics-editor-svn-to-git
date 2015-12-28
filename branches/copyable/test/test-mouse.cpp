// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "geo/canvas-geo.hh"
#include "geo/intrect.hh"
#include "geo/point.hh"
#include "util/mouse.hh"

using namespace faint;
using namespace faint::mouse;

CanvasGeo geo(faint::coord scale, const IntPoint& pos, const IntSize& borderSize){
  CanvasGeo g;
  g.zoom.SetApproximate(scale);
  g.pos = pos;
  g.border = borderSize;
  return g;
}

CanvasGeo geo(faint::coord scale, const IntPoint& pos){
  CanvasGeo g;
  g.zoom.SetApproximate(scale);
  g.pos = pos;
  return g;
}

CanvasGeo geo(faint::coord scale){
  return geo(scale, IntPoint(0,0));
}

CanvasGeo geo(const IntPoint& pos){
  CanvasGeo g;
  g.pos = pos;
  return g;
}

CanvasGeo geo(){
  return CanvasGeo();
}

static void test_image_to_view(){
  VERIFY(geo().border == IntSize(20,20));
  VERIFY(image_to_view(IntPoint(10,10), geo()) == IntPoint(30,30));
  VERIFY(image_to_view(IntPoint(10,10), geo(2.0)) == IntPoint(40,40));
  VERIFY(image_to_view(IntPoint(10,10), geo(2.0, IntPoint(20,70))) == IntPoint(20,-30));
  VERIFY(image_to_view(IntPoint(12,11),
      geo(1.0, IntPoint(10,10), IntSize(30,10))) == IntPoint(32,11));
  VERIFY(image_to_view(IntPoint(10,20),
      geo(2.0, IntPoint(10,10), IntSize(30,10))) == IntPoint(40,40));
}

static void test_image_to_view_rect(){
  VERIFY(image_to_view(IntRect(IntPoint(10,10), IntPoint(20,20)), geo()) ==
    IntRect(IntPoint(30,30), IntPoint(40,40)));

  VERIFY(image_to_view(IntRect(IntPoint(10,10), IntPoint(20,20)),
    geo(2.0, IntPoint(10,10))) == IntRect(IntPoint(30,30), IntPoint(50,50)));

  VERIFY(image_to_view(IntRect(IntPoint(10,10), IntPoint(20,20)),
      geo(2.0, IntPoint(10,10), IntSize(15,25))) ==
    IntRect(IntPoint(25,35), IntPoint(45,55)));
}

static void test_view_to_image(){
  VERIFY(view_to_image(IntPoint(0,0), geo()) == Point(-20, -20));
  VERIFY(view_to_image(IntPoint(40,0), geo(2.0)) == Point(10, -10));

  VERIFY(view_to_image(IntPoint(40,0),
      geo(2.0, IntPoint(10,10))) == Point(15, -5));

  VERIFY(view_to_image(IntPoint(40,0),
      geo(2.0, IntPoint(10,10), IntSize(20,40))) == Point(15, -15));
}

void test_mouse(){
  test_image_to_view();
  test_image_to_view_rect();
  test_view_to_image();

}
