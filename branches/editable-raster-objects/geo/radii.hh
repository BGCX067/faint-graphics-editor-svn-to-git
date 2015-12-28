#ifndef FAINT_RADII_HH
#define FAINT_RADII_HH
#include "geo/basic.hh"

class Radii{
  // Using pretentious "Radii" in favor of "Radiuses", because it
  // sounds less like a list of radiuses and (spuriously) more like a
  // pair of radiuses, which this class is.
public:
  Radii();
  Radii( faint::coord x, faint::coord y );
  bool operator==( const Radii& ) const;
  bool operator!=( const Radii& ) const;
  faint::coord x;
  faint::coord y;
};

#endif
