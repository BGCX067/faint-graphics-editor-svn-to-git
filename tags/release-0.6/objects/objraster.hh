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

#ifndef FAINT_OBJRASTER_HH
#define FAINT_OBJRASTER_HH
#include "bitmap/bitmap.hh"
#include "object.hh"

extern const std::string s_TypeRaster;

class ObjRaster : public Object {
public:
  ObjRaster( const Rect&, const faint::Bitmap&, const Settings& );
  Object* Clone() const;
  void Draw( FaintDC& );
  void DrawMask( FaintDC& );
  std::vector<Point> GetAttachPoints() const;
  IntRect GetRefreshRect();
  bool HitTest( const Point& );

  // Non-virtual
  faint::Bitmap& GetBitmap();
  void SetBitmap( const faint::Bitmap& );
private:
  ObjRaster( const ObjRaster& );
  Rect GetRasterRect() const;
  void OnSetTri();
  faint::Bitmap m_bitmap;
  faint::Bitmap m_scaled;
};

class Command;
Command* crop_raster_object_command( ObjRaster* );

#endif
