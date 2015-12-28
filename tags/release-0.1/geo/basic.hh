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
namespace faint{
  typedef double coord;
  typedef double radian;
  typedef double degree;  
  extern faint::coord coord_epsilon;
}

// Macro for suffixes depending on coordinate-type, e.g.
// f for coord typedef:ed as float, L for long double or 
// nothing for double (current).
#define LITCRD(x) x

inline int truncated( faint::coord c ){
  return static_cast<int>(c);
}

inline int rounded( faint::coord c ){
  return static_cast<int>(c + LITCRD(0.5));
}

inline int rounded_down( faint::coord c ){
  return static_cast<int>(c - LITCRD(0.5));
}

inline faint::coord floated( int v ){
  return static_cast<faint::coord>(v);
}

inline bool coord_eq( faint::coord lhs, faint::coord rhs ){
  return fabs( lhs - rhs ) <= faint::coord_epsilon;
}

inline bool rather_zero( faint::coord c ){
  return fabs(c) <= faint::coord_epsilon;
}

// Declarations for functions that don't exist, to catch cases were
// types have changed so truncation/rounding is uneccessary.
int rounded( int );
int truncated( int );

// Declarations for undefined functions to catch floats used instead of 
// double-based faint::coord
int rounded( float );
int truncated( float );


#endif
