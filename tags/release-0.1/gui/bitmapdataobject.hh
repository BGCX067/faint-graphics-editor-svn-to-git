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
#include <vector>
#include "bitmap/bitmap.h"

class Object;
class wxDataObject;
namespace faint {

class FaintBitmapDataObject : public wxDataObjectSimple {
public:
  FaintBitmapDataObject();
  explicit FaintBitmapDataObject( const faint::Bitmap& bmp );
  virtual ~FaintBitmapDataObject();

  // Inherited
  wxDataFormat GetFormat() const;  
  
  size_t GetDataSize() const;
  bool GetDataHere( void* buf ) const;
  bool SetData( size_t len, const void* buf );

  // Custom
  faint::Bitmap GetBitmap() const;

private:
  FaintBitmapDataObject( const FaintBitmapDataObject& );
  faint::Bitmap m_bmp;
};

}
#endif
