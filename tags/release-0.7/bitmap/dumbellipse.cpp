#include "bitmap/bitmap.hh"
using std::swap;
namespace faint{

void draw_dumb_ellipse( faint::Bitmap& bmp, const IntRect& r, const faint::Color& c ){
  IntPoint radiuses(truncated(floated(IntPoint(r.w, r.h)) / LITCRD(2.0)));
  int x0 = r.x + radiuses.x;
  int y0 = r.y + radiuses.y;
  int a = radiuses.x;
  int b = radiuses.y;

  if ( a == 0 ){
    draw_line( bmp, r.TopLeft(), r.BottomLeft(), c, 1, false, LineCap::BUTT );
    return;
  }
  else if ( b == 0 ){
    draw_line( bmp, r.TopLeft(), r.TopRight(), c, 1, false, LineCap::BUTT );
    return;
  }

  int a2 = 2 * a * a;
  int b2 = 2 * b * b;
  int error = a * a * b;
  int x = 0;
  int y = b;

  int stopy = 0;
  int stopx = a2 * b;

  int xoffset = r.w % 2 == 0 ? 1 : 0;
  int yoffset = r.h % 2 == 0 ? 1 : 0;
  bool go = false;
  IntPoint center(r.x + r.w / 2, r.y + r.h / 2);
  IntPoint prev(r.x + r.w / 2, r.y);
  while (stopy <= stopx) {
    if ( go ){
      draw_line(bmp, IntPoint(x0 + x - xoffset, y0 + y - yoffset), center, c, 1, false, LineCap::BUTT);
      draw_line(bmp, IntPoint(x0 - x, y0 + y - yoffset), center, c, 1, false, LineCap::BUTT );
      draw_line(bmp, IntPoint(x0 - x, y0 - y), center, c, 1, false, LineCap::BUTT );
      draw_line(bmp, IntPoint(x0 + x - xoffset, y0 - y), center, c, 1, false, LineCap::BUTT );
      go = false;
    }
    else {
      put_pixel_raw( bmp, x0 + x - xoffset, y0 + y - yoffset, c );
      put_pixel_raw( bmp, x0 - x, y0 + y - yoffset, c );
      put_pixel_raw( bmp, x0 - x, y0 - y, c );
      put_pixel_raw( bmp, x0 + x - xoffset, y0 - y, c );
    }
    x++;

    error -= b2 * (x - 1);
    stopy += b2;
    if (error <= 0) {
      error += a2 * (y - 1);
      y--;
      go = true;
      stopx -= a2;
    }
  }

  error = b*b*a;
  x = a;
  y = 0;
  stopy = b2*a;
  stopx = 0;
  go = false;
  while (stopy >= stopx) {
    if ( go ){
      draw_line(bmp, IntPoint(x0 + x - xoffset, y0 + y - yoffset), center, c, 1, false, LineCap::BUTT);
      draw_line(bmp, IntPoint(x0 - x, y0 + y - yoffset), center, c, 1, false, LineCap::BUTT );
      draw_line(bmp, IntPoint(x0 - x, y0 - y), center, c, 1, false, LineCap::BUTT );
      draw_line(bmp, IntPoint(x0 + x - xoffset, y0 - y), center, c, 1, false, LineCap::BUTT );
      go = false;
    }
    else{
      put_pixel_raw( bmp, x0 + x - xoffset, y0 + y - yoffset, c);
      put_pixel_raw( bmp, x0 - x, y0 + y - yoffset, c);
      put_pixel_raw( bmp, x0 - x, y0 - y, c);
      put_pixel_raw( bmp, x0 + x - xoffset, y0 - y, c);
    }
    y++;
    error -= a2 * (y - 1);
    stopx += a2;
    if (error < 0) {
      go = true;
      error += b2 * (x - 1);
      x--;
      stopy -= b2;
    }
  }
}

} // namespace
