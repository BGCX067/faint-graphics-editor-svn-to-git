// -*- coding: us-ascii-unix -*-
#ifndef FAINT_TEST_UTIL_PRINT_OBJECTS_HH
#define FAINT_TEST_UTIL_PRINT_OBJECTS_HH
#include <iosfwd>

namespace faint{

class IntPoint;
std::ostream& operator<<(std::ostream&, const IntPoint&);

class IntRect;
std::ostream& operator<<(std::ostream&, const IntRect&);

class IntSize;
std::ostream& operator<<(std::ostream&, const IntSize&);

class Point;
std::ostream& operator<<(std::ostream&, const Point&);

class utf8_char;
std::ostream& operator<<(std::ostream&, const utf8_char&);

class Color;
std::ostream& operator<<(std::ostream&, const Color&);

class ColRGB;
std::ostream& operator<<(std::ostream&, const ColRGB&);

class index_t;
std::ostream& operator<<(std::ostream&, const index_t&);

// Avoids unsigned char values being casted to char and printed as
// garbage ascii.
std::ostream& operator<<(std::ostream&, unsigned char);

} // namespace

#endif
