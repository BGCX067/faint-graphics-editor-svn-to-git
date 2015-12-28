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

#ifndef FAINT_FILTER_HH
#include "bitmap/bitmap.hh"
class Filter;

namespace faint{
  void blur(faint::Bitmap&);
  void desaturate_simple( Bitmap& );
  void desaturate_weighted( Bitmap& );
  Filter* get_blur_filter();
  Filter* get_stroke_filter();
  void invert( Bitmap& );
}

#endif
