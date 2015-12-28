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

#ifndef FAINT_MOUSE_HH
#define FAINT_MOUSE_HH

class wxWindow;
namespace faint {
  namespace mouse {
    IntPoint screen_position();
    IntPoint view_position( const wxWindow& );
    Point image_position( const Geometry&, const wxWindow& );
    Point view_to_image( const IntPoint&, const Geometry& );
    IntPoint image_to_view( const IntPoint&, const Geometry& );
    IntRect image_to_view( const IntRect&, const Geometry& );
    IntPoint image_to_view_tr( const Point&, const Geometry& );
    IntRect image_to_view_tr( const Rect&, const Geometry& );
  }
}

#endif
