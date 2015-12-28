#include "geo/radii.hh"

Radii::Radii()
  : x(0.0),
    y(0.0)
{}

Radii::Radii( faint::coord in_x, faint::coord in_y )
  : x(in_x),
    y(in_y)
{}

bool Radii::operator==( const Radii& other ) const{
  return coord_eq(x, other.x) && coord_eq(y, other.y);
}

bool Radii::operator!=( const Radii& other ) const{
  return !operator==(other);
}
