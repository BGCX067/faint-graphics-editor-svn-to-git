// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#ifndef FAINT_RADII_HH
#define FAINT_RADII_HH
#include "geo/primitive.hh"

namespace faint{

class Radii{
  // Using pretentious "Radii" in favor of "Radiuses", because it
  // sounds less like a list of radiuses and (spuriously) more like a
  // pair of radiuses, which this class is.
public:
  Radii();
  Radii(coord x, coord y);
  bool operator==(const Radii&) const;
  bool operator!=(const Radii&) const;
  void operator*=(coord);
  coord x;
  coord y;
};

Radii abs(const Radii&);
Radii operator*(const Radii&, const Radii&);

} // namespace

#endif
