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

#include "commands/set-bitmap-cmd.hh"
#include "util/objutil.hh"

// SetObjectBitmapCommand
SetObjectBitmapCommand::SetObjectBitmapCommand(ObjRaster* object, const faint::Bitmap& bmp, const Tri& tri, const std::string& name )
  : Command( CommandType::OBJECT ),
    m_object( object ),
    m_bitmap( bmp ),
    m_oldBitmap( object->GetBitmap() ),
    m_tri( tri ),
    m_oldTri( object->GetTri() ),
    m_name(name)
{}

void SetObjectBitmapCommand::Do( CommandContext& ){
  m_object->SetBitmap( m_bitmap );
  m_object->SetTri( m_tri );
}

std::string SetObjectBitmapCommand::Name() const{
  return m_name;
}

void SetObjectBitmapCommand::Undo( CommandContext& ){
  m_object->SetBitmap( m_oldBitmap );
  m_object->SetTri( m_oldTri );
}

// SetBitmapCommand
SetBitmapCommand::SetBitmapCommand(const faint::Bitmap& bmp, const IntPoint& topLeft, const std::string& name)
  : Command( CommandType::HYBRID ),
    m_bitmap(bmp),
    m_name(name),
    m_topLeft(topLeft)
{}

void SetBitmapCommand::Do( CommandContext& context ){
  DoRaster(context);
  offset_by(context.GetObjects(), -m_topLeft);
}

void SetBitmapCommand::DoRaster( CommandContext& context ){
  context.SetBitmap( m_bitmap );
}

std::string SetBitmapCommand::Name() const{
  return m_name;
}

void SetBitmapCommand::Undo( CommandContext& context ){
  offset_by(context.GetObjects(), m_topLeft );
}
