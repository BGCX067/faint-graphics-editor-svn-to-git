// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#ifndef FAINT_PRIMITIVE_HH
#define FAINT_PRIMITIVE_HH
#include <cstddef>

namespace faint{
typedef double coord;
typedef double degree;
typedef double radian;
typedef unsigned int uint;
typedef unsigned char uchar;
extern const coord coord_epsilon;

bool coord_eq(coord, coord);
int ceiled(coord);
int floored(coord);
int floored(int) = delete;

// Converts to size_t. Asserts if < 0
size_t to_size_t(int);

// Catch redundant conversions
size_t to_size_t(size_t) = delete;

// Conversion from size_t to int. For idiosyncratic reasons, I'm
// trying to migrate to using size_t less, using these functions
// instead of a raw static_cast will point me to conversions that are
// no longer required.
int resigned(size_t);
int resigned(int) = delete;
int resigned(coord) = delete;

coord floated(int);
int truncated(coord);
bool rather_zero(coord);
int rounded(coord);
int rounded_down(coord);

// Catch cases where types have changed so truncation/rounding is
// unnecessary
int rounded(int) = delete;
int truncated(int) = delete;

// Prevent using floats instead of double-based coord
int rounded(float) = delete;
int truncated(float) = delete;

}

#endif
