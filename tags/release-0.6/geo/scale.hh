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

#ifndef FAINT_SCALE_HH
#define FAINT_SCALE_HH
#include "geo/size.hh"
#include "util/unique.hh"

typedef Order<Size>::New NewSize;

// Fixme: Extend to prevent construction from ints, add IntScale and floated etc.
class Scale{
public:
  Scale( faint::coord x, faint::coord y );
  Scale( faint::coord xy );
  Scale( const NewSize&, const Size& );
  faint::coord x;
  faint::coord y;
private:
};

Scale invert_x_scale();
Scale invert_y_scale();

#endif
