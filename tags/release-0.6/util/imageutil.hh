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

#ifndef FAINT_IMAGEUTIL_HH
#define FAINT_IMAGEUTIL_HH
#include "util/commonfwd.hh"

namespace faint{
  bool can_move_backward( const Image&, const objects_t& );
  bool can_move_forward( const Image&, const objects_t& );
  bool has_all( const Image&, const objects_t& );
  bool has_objects( const Image&, const objects_t& );

  // Returns the background from the image with the selection
  // stamped onto it. The bitmap will not contain the objects.
  // Use to avoids losing the floating raster selection graphic when
  // saving to vector formats with a raster background. 
  Bitmap stamp_selection( const Image& );

  // Returns a bitmap with the background from the image with all
  // objects and any floating selection stamped onto it.
  // Use to include objects and the floating raster selection graphic
  // when saving to raster formats. 
  Bitmap flatten( const Image& );

  // Gets the highest Z-value in the image (the front-most object).
  // Asserts that the image has objects.
  size_t get_highest_z( const faint::Image& );
  void remove_missing_objects_from( objects_t&, const Image& );
}

#endif
