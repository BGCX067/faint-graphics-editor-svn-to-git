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

#include <new> // for bad_alloc
#include "cairo.h"
#include "geo/int-size.hh"

namespace faint{
  int faint_cairo_stride(const IntSize& sz){
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, sz.w);
    if (stride == -1){
      // cairo_format_stride_for_width returns -1 if "the format is invalid
      // or the width too large".
      //
      // Most likely the width was too large (not sure what that means
      // though). By throwing bad_alloc here, Bitmap constructors will
      // throw bad_alloc for this case, just like they do from the
      // new-call if the combined size is too large for memory.
      throw std::bad_alloc();
    }
    return stride;
  }
}
