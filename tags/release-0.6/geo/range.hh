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
#ifndef FAINT_RANGE_HH
#define FAINT_RANGE_HH
#include "util/unique.hh"

class IntRange;
typedef Unique<int, IntRange, 0> min_t;
typedef Unique<int, IntRange, 1> max_t;

class IntRange{
public:
  // Creates an unlimited range, Constrain():s nothing,
  // Has() everything.
  IntRange();

  // A range between min and max.
  // Asserts that min <= max.
  IntRange(const min_t&, const max_t&);

  IntRange(const min_t&); // Lower bound only
  IntRange(const max_t&); // Upper bound only

  // Return the value constrained to the range.
  int Constrain( int ) const;

  // True if the value is within the range
  bool Has( int ) const;
private:
  Optional<int> m_max;
  Optional<int> m_min;
};

#endif
