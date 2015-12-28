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

#include "wx/dataobj.h"
#include "wx/dnd.h"
#include "gui/bitmapdataobject.hh"

namespace faint {
FaintBitmapDataObject::FaintBitmapDataObject( const faint::Bitmap& bmp )
  : wxDataObjectSimple( wxDataFormat("FaintBitmap" ) ),
    m_bmp(bmp)
{}

FaintBitmapDataObject::FaintBitmapDataObject()
  : wxDataObjectSimple( wxDataFormat("FaintBitmap" ) )
{}

faint::Bitmap FaintBitmapDataObject::GetBitmap() const{
  return m_bmp;
}

bool FaintBitmapDataObject::GetDataHere( void *buf ) const{
  // Fixme: memleak. Instead of copying a faint::Bitmap, I should copy
  // the data buffer content
  faint::Bitmap* bmp = new faint::Bitmap( m_bmp );
  memcpy( buf, bmp, sizeof(faint::Bitmap) );
  return true;
}

size_t FaintBitmapDataObject::GetDataSize() const{
  return sizeof( faint::Bitmap );
}

bool FaintBitmapDataObject::SetData( size_t len, const void* buf ){
  if (len != sizeof(faint::Bitmap ) ){
    return false;
  }
  faint::Bitmap tmp;
  memcpy(&tmp, buf, len );
  m_bmp = tmp;
  return true;
}

bool FaintBitmapDataObject::SetData( const wxDataFormat&, size_t len, const void* buf ){
  return SetData(len, buf);
}

} // namespace faint
