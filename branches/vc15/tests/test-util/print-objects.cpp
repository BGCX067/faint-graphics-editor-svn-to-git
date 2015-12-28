// -*- coding: us-ascii-unix -*-
#include <iomanip>
#include <ostream>
#include "bitmap/color.hh"
#include "geo/int-point.hh"
#include "geo/int-rect.hh"
#include "geo/int-size.hh"
#include "geo/point.hh"
#include "tests/test-util/print-objects.hh"
#include "text/utf8-char.hh"
#include "text/formatting.hh"

namespace faint {

std::ostream& operator<<(std::ostream& o, const IntRect& r){
  return o << r.x << "," << r.y << "," << r.w << "," << r.h;
}

std::ostream& operator<<(std::ostream& o, const utf8_char& ch){
  const auto oldFill = o.fill();
  return o <<
    "U+" <<
    std::hex <<
    std::setw(4) <<
    std::setfill('0') <<
    ch.codepoint() <<
    std::setfill(oldFill);
}

std::ostream& operator<<(std::ostream& o, const IntPoint& pt){
  return o << pt.x << "," << pt.y;
}

std::ostream& operator<<(std::ostream& o, const IntSize& sz){
  return o << sz.w << "," << sz.h;
}

std::ostream& operator<<(std::ostream& o, const Point& pt){
  return o << pt.x << "," << pt.y;
}

std::ostream& operator<<(std::ostream& o, const Color& c){
  return o << str_rgba(c);
}

std::ostream& operator<<(std::ostream& o, const ColRGB& c){
  return o << str_rgb(Color(c, 255));
}

std::ostream& operator<<(std::ostream& o, const index_t& i){
  return o << str_coder(i);
}

std::ostream& operator<<(std::ostream& o, unsigned char v){
  return o << static_cast<unsigned int>(v);
}

} // namespace
