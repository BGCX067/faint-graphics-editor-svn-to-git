// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "app/canvas-handle.hh"

void test_canvas_handle(){
  using namespace faint;

  using HP = HandlePos;
  VERIFY(opposite_handle_pos(HP::TOP_LEFT) == HP::BOTTOM_RIGHT);
  VERIFY(opposite_handle_pos(HP::TOP_SIDE) == HP::BOTTOM_SIDE);
  VERIFY(opposite_handle_pos(HP::TOP_RIGHT) == HP::BOTTOM_LEFT);
  VERIFY(opposite_handle_pos(HP::RIGHT_SIDE) == HP::LEFT_SIDE);
  VERIFY(opposite_handle_pos(HP::BOTTOM_RIGHT) == HP::TOP_LEFT);
  VERIFY(opposite_handle_pos(HP::BOTTOM_SIDE) == HP::TOP_SIDE);
  VERIFY(opposite_handle_pos(HP::BOTTOM_LEFT) == HP::TOP_RIGHT);
  VERIFY(opposite_handle_pos(HP::LEFT_SIDE) == HP::RIGHT_SIDE);

  using HD = HandleDirection;
  VERIFY(handle_direction(HP::TOP_LEFT) == HD::DIAGONAL);
  VERIFY(handle_direction(HP::TOP_SIDE) == HD::UP_DOWN);
  VERIFY(handle_direction(HP::TOP_RIGHT) == HD::DIAGONAL);
  VERIFY(handle_direction(HP::RIGHT_SIDE) == HD::LEFT_RIGHT);
  VERIFY(handle_direction(HP::BOTTOM_RIGHT) == HD::DIAGONAL);
  VERIFY(handle_direction(HP::BOTTOM_SIDE) == HD::UP_DOWN);
  VERIFY(handle_direction(HP::BOTTOM_LEFT) == HD::DIAGONAL);
  VERIFY(handle_direction(HP::LEFT_SIDE) == HD::LEFT_RIGHT);

  const IntSize sz(100,70);
  const CanvasGeo g;
  // Top left handle, upper left corner
  VERIFY(canvas_handle_hit_test(IntPoint(6,7), sz, g).NotSet());
  VERIFY(canvas_handle_hit_test(IntPoint(7,6), sz, g).NotSet());
  VERIFY(canvas_handle_hit_test(IntPoint(7,7), sz, g).IsSet());

  // Top left handle, Lower right corner
  VERIFY(canvas_handle_hit_test(IntPoint(12,12), sz, g).IsSet());
  VERIFY(canvas_handle_hit_test(IntPoint(13,12), sz, g).NotSet());
  VERIFY(canvas_handle_hit_test(IntPoint(12,13), sz, g).NotSet());

  auto topLeft(canvas_handle_hit_test(IntPoint(10,10), sz, g));
  ASSERT(topLeft.IsSet());
  VERIFY(topLeft.Get().GetDirection() == HD::DIAGONAL);
  VERIFY(topLeft.Get().Opposite().GetRect() ==
    IntRect(IntPoint(127,97), IntSize(6,6)));
}
