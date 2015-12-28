// Copyright 2012 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#ifndef FAINT_ANGLE_HH
#define FAINT_ANGLE_HH
#include "geo/geotypes.hh"
#include <cmath>

namespace faint{
  extern const faint::radian pi;
  extern const faint::degree degreePerRadian;
}

inline faint::coord radius(faint::coord x0, faint::coord y0, faint::coord x1, faint::coord y1){
  return sqrt( (x0 - x1 ) * (x0 - x1 ) + ( y0 - y1 ) * ( y0 - y1 ) );
}

inline faint::radian rad_angle( faint::coord x0, faint::coord y0, faint::coord x1, faint::coord y1 ){
  faint::coord x1b = x1 - x0;
  faint::coord y1b = y1 - y0;
  return atan2( y1b, x1b );
}

inline faint::degree deg_angle( faint::coord x0, faint::coord y0, faint::coord x1, faint::coord y1 ){
  if ( x0 == x1 && y0 == y1 ){
    return 0;
  }

  faint::coord r_angle = rad_angle( x0, y0, x1, y1 );
  return r_angle * faint::degreePerRadian;
}

inline faint::degree angle360( faint::coord x0, faint::coord y0, faint::coord x1, faint::coord y1 ){
  if ( x0 == x1 && y0 == y1 ){
    return 0;
  }
  faint::radian r_angle = rad_angle( x0, y0, x1, y1 );
  faint::degree d_angle = r_angle * faint::degreePerRadian;
  if ( d_angle < 0 ){
    d_angle = 360.0 + d_angle;
  }
  return ( d_angle == 0 ? 0.0 : 360.0 - d_angle );
}

#endif
