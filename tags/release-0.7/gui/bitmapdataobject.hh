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

#ifndef FAINT_BITMAPDATAOBJECT_HH
#define FAINT_BITMAPDATAOBJECT_HH
#include "wx/clipbrd.h"
#include "bitmap/bitmap.hh"

namespace faint {

class FaintBitmapDataObject : public wxDataObjectSimple {
public:
  FaintBitmapDataObject();
  explicit FaintBitmapDataObject( const faint::Bitmap& );
  faint::Bitmap GetBitmap() const;
  bool GetDataHere( void* buf ) const;  // Inherited
  size_t GetDataSize() const;  // Inherited
  wxDataFormat GetFormat() const; // Inherited
  bool SetData( size_t len, const void* buf );  // Inherited
  bool SetData(const wxDataFormat&, size_t len, const void*); // Inherited

private:
  FaintBitmapDataObject( const FaintBitmapDataObject& );
  faint::Bitmap m_bmp;
};

}
#endif
