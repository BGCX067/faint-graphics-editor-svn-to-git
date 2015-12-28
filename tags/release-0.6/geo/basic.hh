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

#ifndef FAINT_BASIC_HH
#define FAINT_BASIC_HH
#include <cmath>
#include "util/commonfwd.hh"

// Macro for suffixes depending on coordinate-type, e.g.
// f for coord typedef:ed as float, L for long double or
// nothing for double (current).
#define LITCRD(x) x

namespace faint{
  inline int iabs( int i ){
    return i < 0 ? -i : i;
  }
  faint::coord iabs( faint::coord ); // Not defined
}
inline bool coord_eq( faint::coord lhs, faint::coord rhs ){
  return fabs( lhs - rhs ) <= faint::coord_epsilon;
}

inline faint::coord floated( int v ){
  return static_cast<faint::coord>(v);
}

inline int truncated( faint::coord c ){
  return static_cast<int>(c);
}

inline bool rather_zero( faint::coord c ){
  return fabs(c) <= faint::coord_epsilon;
}

inline int round_away_from_zero( faint::coord c ){
  return static_cast<int>(c < 0 ? c - 0.5 : c + 0.5);
}

IntPoint round_away_from_zero( const Point& );

inline int rounded( faint::coord c ){
  return static_cast<int>(c + LITCRD(0.5));
}

inline int rounded_down( faint::coord c ){
  return static_cast<int>(c - LITCRD(0.5));
}

// Declarations for functions that don't exist, to catch cases where
// types have changed so truncation/rounding is uneccessary.
int rounded( int ); // undefined
int truncated( int ); // undefined

// Declarations for undefined functions to catch floats used instead
// of double-based faint::coord.
int rounded( float ); // undefined
int truncated( float ); // undefined

#endif
