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

#include <algorithm>
#include <cassert>
#include <functional>
#include "objects/object.hh"
#include "rendering/faintdc.hh"
#include "util/commandutil.hh"
#include "util/image.hh"

namespace faint{

bool can_move_backward( const Image& image, const objects_t& objects ){
  if ( objects.empty() ){
    return false;
  }
  Command* cmd = get_objects_backward_command( objects, image );
  if ( cmd != nullptr ){
    delete cmd;
    return true;
  }
  return false;
}

bool can_move_forward( const Image& image, const objects_t& objects ){
  if ( objects.empty() ){
    return false;
  }
  Command* cmd = get_objects_forward_command( objects, image );
  if ( cmd != nullptr ){
    delete cmd;
    return true;
  }
  return false;
}

void stamp_selection_impl(FaintDC& dc, const faint::Image& image ){
  const RasterSelection& selection = image.GetRasterSelection();
  selection.DrawFloating(dc);
}

faint::Bitmap stamp_selection( const faint::Image& image ){
  faint::Bitmap bmp(image.GetBitmap());
  FaintDC dc( bmp );
  stamp_selection_impl(dc, image);
  return bmp;
}

faint::Bitmap flatten( const faint::Image& image ){
  faint::Bitmap bmp(image.GetBitmap());
  FaintDC dc( bmp );
  for ( Object* obj : image.GetObjects() ){
    obj->Draw( dc );
  }
  stamp_selection_impl(dc, image);
  return bmp;
}

size_t get_highest_z( const faint::Image& image ){
  size_t numObjects = image.GetNumObjects();
  assert( numObjects != 0 );
  return numObjects - 1;
}

bool has_all( const Image& image, const objects_t& objects ){
  for ( const Object* obj : objects ){
    if ( !image.HasObject(obj) ){
      return false;
    }
  }
  return true;
}

bool has_objects( const faint::Image& image ){
  return !image.GetObjects().empty();
}

void remove_missing_objects_from( objects_t& objects, const Image& image ){
  auto object_not_in_image = [&image](const Object* obj){ return !image.HasObject(obj); };
  objects.erase(remove_if( objects.begin(), objects.end(), object_not_in_image ), objects.end());
}

} // namespace faint
