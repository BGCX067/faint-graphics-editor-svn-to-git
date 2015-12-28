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
#include "commands/resize-cmd.hh"
#include "objects/object.hh"
#include "util/objutil.hh"

static void offset_for_resize( CommandContext& context, const IntPoint& offset ){
  offset_by( context.GetObjects(), offset );
  context.OffsetRasterSelectionOrigin( offset );
}

ResizeCommand::ResizeCommand( const IntRect& rect, const faint::DrawSource& bg, const std::string& name )
  : Command(CommandType::HYBRID),
    m_bg( bg ),
    m_isDwim(false),
    m_name(name),
    m_rect( rect )
{}

ResizeCommand::ResizeCommand( const IntRect& rect, const AltIntRect& altRect, const faint::DrawSource& bg, const std::string& name )
  : Command(CommandType::HYBRID),
    m_altRect(altRect.Get()),
    m_bg( bg ),
    m_isDwim(false),
    m_name(name),
    m_rect(rect)
{}

ResizeCommand::ResizeCommand( const IntRect& rect, const faint::DrawSource& bg, const AltBgColor& altBg, const std::string& name )
  : Command(CommandType::HYBRID),
    m_altBg(altBg.Get()),
    m_bg( bg ),
    m_isDwim(false),
    m_name(name),
    m_rect(rect)
{}

void ResizeCommand::Do( CommandContext& context ){
  DoRaster( context );
  offset_for_resize(context, -m_rect.TopLeft());
}

void ResizeCommand::DoRaster( CommandContext& context ){
  faint::Bitmap bmp(m_rect.GetSize());
  clear(bmp, m_bg);
  blit( context.GetBitmap(), onto(bmp), -m_rect.TopLeft() );
  context.SetBitmap(bmp);
}

Command* ResizeCommand::GetDWIM(){
  if ( m_altBg.IsSet() ){
    ResizeCommand* cmd = new ResizeCommand( m_rect, m_altBg.Get(), Alternate(m_bg), "Resize Image (DWIM)");
    cmd->SetDWIM();
    return cmd;
  }
  else if ( m_altRect.IsSet() ){
    ResizeCommand* cmd = new ResizeCommand( m_altRect.Get(), Alternate(m_rect), m_bg, "Resize Image (DWIM)");
    cmd->SetDWIM();
    return cmd;
  }
  assert( false );
  return nullptr;
}

bool ResizeCommand::HasDWIM() const{
  return m_altBg.IsSet() || m_altRect.IsSet();
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
  offset_for_resize(context, m_rect.TopLeft());
}
