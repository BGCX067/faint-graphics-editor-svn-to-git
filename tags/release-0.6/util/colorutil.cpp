#include "colorutil.hh"
#include <cassert>
bool red_is_max( const faint::Color& c ){
  return c.r >= c.g && c.r >= c.b;
}

bool green_is_max( const faint::Color& c ){
  return c.g >= c.r && c.g >= c.b;
}

bool blue_is_max( const faint::Color& c ){
  return c.b >= c.r && c.b >= c.g;
}

int color_max( const faint::Color& c ){
  return std::max(std::max(c.r, c.g), c.b);
}

int color_min( const faint::Color& c ){
  return std::min(std::min(c.r, c.g), c.b);
}

double get_hp( const faint::Color& color, const double C ){
  int r = color.r;
  int g = color.g;
  int b = color.b;
  if ( red_is_max(color) ){
    return (g - b) / C + 6;
  }
  else if ( green_is_max(color) ){
    return (b - r) / C + 2;
  }
  else if ( blue_is_max(color) ){
    return ( r - g ) / C + 4;
  }
  assert(false);
  return 0;
}

int get_hue( const faint::Color& color ){
  const int M = color_max(color);
  const int m = color_min(color);
  const double C = M - m;
  if ( C == 0 ){
    // Undefined
    return 0;
  }

  double hp = get_hp( color, C );
  const double h = 60 * hp;
  return int((h / 360.0) * 240 + 0.5) % 240;
}

int get_lightness( const faint::Color& c ){
  const int M = color_max(c);
  const int m = color_min(c);
  return static_cast<int>(((( M + m ) / 2.0) / 255.0) * 240);
}

double get_raw_lightness( const faint::Color& c ){
  const int M = color_max(c);
  const int m = color_min(c);
  return ( M + m ) / 2.0;
}

int get_saturation( const faint::Color& color ){
  const int M = color_max(color);
  const int m = color_min(color);
  double C = M - m;
  if ( C == 0 ){
    return 0;
  }

  return static_cast<int>((C / (240 - std::abs(2 * get_lightness(color) - 240))) * 240);
}
