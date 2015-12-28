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
  if ( cmd != 0 ){
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
  if ( cmd != 0 ){
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
  objects_t objs = image.GetObjects();
  for ( size_t i = 0; i != objs.size(); i++ ){
    objs[i]->Draw( dc );
  }
  stamp_selection_impl(dc, image);
  return bmp;
}

size_t get_highest_z( const faint::Image& image ){
  size_t numObjects = image.GetNumObjects();
  assert( numObjects != 0 );
  return numObjects - 1;
}

bool has_all( const Image& image, const objects_t& objs ){
  for ( objects_t::const_iterator it = objs.begin(); it != objs.end(); ++it ){
    if ( !image.HasObject(*it) ){
      return false;
    }
  }
  return true;
}

bool has_objects( const faint::Image& image ){
  return !image.GetObjects().empty();
}

struct obj_exists_in{
  obj_exists_in( const faint::Image& img )
    : m_image(img)
  {}
  bool operator()(Object* obj) const{
    return m_image.HasObject(obj);
  }
  typedef Object* argument_type;
private:
  obj_exists_in& operator=(const obj_exists_in&);
  const faint::Image& m_image;
};

void remove_missing_objects_from( objects_t& objects, const Image& image ){
  objects_t::iterator newEnd = std::remove_if( objects.begin(), objects.end(),
    std::not1(obj_exists_in(image)));
  objects.erase(newEnd, objects.end() );
}

} // namespace faint
