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

#include "cmdsetbitmap.hh"
#include "faintimage.hh"

// SetObjectBitmapCommand
SetObjectBitmapCommand::SetObjectBitmapCommand(ObjRaster* object, const faint::Bitmap& bmp, const Tri& tri )
  : Command( CMD_TYPE_OBJECT ),
    m_object( object ),
    m_bitmap( bmp ),
    m_oldBitmap( object->GetBitmap() ),
    m_tri( tri ),
    m_oldTri( object->GetTri() )
{}

void SetObjectBitmapCommand::Do( faint::Image& ){
  m_object->SetBitmap( m_bitmap );
  m_object->SetTri( m_tri );
}

void SetObjectBitmapCommand::Undo( faint::Image& ){
  m_object->SetBitmap( m_oldBitmap );
  m_object->SetTri( m_oldTri );
}

// SetBitmapCommand
SetBitmapCommand::SetBitmapCommand(const faint::Bitmap& bmp)
  : Command( CMD_TYPE_RASTER ),
    m_bitmap(bmp)
{}

void SetBitmapCommand::Do( faint::Image& image ){
  image.SetBitmap( m_bitmap );
}


