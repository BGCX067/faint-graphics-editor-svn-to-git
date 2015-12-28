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

#include <cassert>
#include "commands/resizecommand.hh"
#include "objects/object.hh"
#include "rendering/cairocontext.hh"
#include "util/objutil.hh"

ResizeCommand::ResizeCommand( const IntRect& rect, const faint::Color& bgColor, const std::string& name )
  : Command(CMD_TYPE_HYBRID),
    m_bgColor( bgColor ),
    m_isDwim(false),
    m_name(name),
    m_rect( rect )
{}

ResizeCommand::ResizeCommand( const IntRect& rect, const AltIntRect& altRect, const faint::Color& bgColor, const std::string& name )
  : Command(CMD_TYPE_HYBRID),
    m_alternateRect(altRect.Get()),
    m_bgColor( bgColor ),
    m_isDwim(false),
    m_name(name),
    m_rect(rect)
{}

ResizeCommand::ResizeCommand( const IntRect& rect, const faint::Color& bgColor, const AltBgColor& altColor, const std::string& name )
  : Command(CMD_TYPE_HYBRID),
    m_alternateColor(altColor.Get()),
    m_bgColor( bgColor ),
    m_isDwim(false),
    m_name(name),
    m_rect(rect)
{}

void ResizeCommand::Do( CommandContext& context ){
  DoRaster( context );
  offset_by( context.GetObjects(), -m_rect.TopLeft() );
}

void ResizeCommand::DoRaster( CommandContext& context ){
  faint::Bitmap bmp( faint::cairo_compatible_bitmap(m_rect.GetSize()));
  clear( bmp, m_bgColor );
  blit( context.GetBitmap(), onto(bmp), -m_rect.TopLeft() );
  context.SetBitmap(bmp);
}

Command* ResizeCommand::GetDWIM(){
  if ( m_alternateColor.IsSet() ){
    ResizeCommand* cmd = new ResizeCommand( m_rect, m_alternateColor.Get(), Alternate(m_bgColor), "Resize Image (DWIM)");
    cmd->SetDWIM();
    return cmd;
  }
  else if ( m_alternateRect.IsSet() ){
    ResizeCommand* cmd = new ResizeCommand( m_alternateRect.Get(), Alternate(m_rect), m_bgColor, "Resize Image (DWIM)");
    cmd->SetDWIM();
    return cmd;
  }
  assert( false );
  return 0;
}

bool ResizeCommand::HasDWIM() const{
  return m_alternateColor.IsSet() || m_alternateRect.IsSet();
}

std::string ResizeCommand::Name() const{
  return m_name;
}

IntPoint ResizeCommand::SelectionOffset() const{
  return -m_rect.TopLeft();
}

void ResizeCommand::SetDWIM(){
  m_isDwim = true;
}

Point ResizeCommand::Translate(const Point& p) const{
  if ( m_isDwim ){
    // Do not reposition if DWIM, as the repositioning has already been done,
    // (assuming the scrolling due to commands is not undone).
    return p;
  }
  return p - floated(m_rect.TopLeft());
}

void ResizeCommand::Undo( CommandContext& context ){
  offset_by(context.GetObjects(), m_rect.TopLeft() );
}
