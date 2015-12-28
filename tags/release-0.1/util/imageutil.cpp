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

#include "faintimage.hh"
#include "faintdc.hh"
#include "objects/object.hh"
namespace faint{

faint::Bitmap Flatten( const faint::Image& image ){
  faint::Bitmap bmp(image.GetBitmapRef());
  FaintDC dc( bmp );
  std::vector<Object*> objs = image.GetObjects();
  for ( size_t i = 0; i != objs.size(); i++ ){
    objs[i]->Draw( dc );
  }
  return bmp;
}

} // Namespace faint
