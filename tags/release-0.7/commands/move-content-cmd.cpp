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

#include "commands/move-content-cmd.hh"
#include "geo/tri.hh"
#include "rendering/faintdc.hh"
#include "util/settingutil.hh"

CmdMoveContent::CmdMoveContent( const faint::Bitmap& bitmap, const NewIntRect& newRect, const OldIntRect& oldRect,
  const copy_content& copy, const Settings& s, const std::string& name )
  : Command( CMD_TYPE_RASTER ),
    m_bitmap( bitmap ),
    m_copy( copy.Get() ),
    m_oldRect( oldRect.Get() ),
    m_rect(newRect.Get()),
    m_settings(s),
    m_name(name)
{}

void CmdMoveContent::Do( CommandContext& context ){
  FaintDC& dc( context.GetDC() );
  if ( ! m_copy ){
    dc.Rectangle( tri_from_rect(floated(m_oldRect)), eraser_rectangle_settings(m_settings.Get(ts_BgCol)));
  }
  dc.Bitmap( m_bitmap, floated(m_rect.TopLeft()), m_settings );
}

std::string CmdMoveContent::Name() const{
  return m_name;
}
