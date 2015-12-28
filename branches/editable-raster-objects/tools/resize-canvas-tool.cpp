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
#include "rendering/overlay.hh"
#include "tools/resize-canvas-tool.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

static bool can_constrain( HandleDirection dir ){
  return dir == HandleDirection::DIAGONAL;
}

static bool constrain_resize( int modifiers, HandleDirection dir ){
  return fl(TOOLMODIFIER2, modifiers) && can_constrain(dir);
}

static IntRect get_resize_rect( const CanvasResizeHandle& handle, const IntPoint& release){
  HandleDirection dir = handle.GetDirection();
  IntPoint opposite = handle.Opposite().GetPos();
  IntSize imageSize = handle.GetCanvasSize();
  if ( dir == HandleDirection::DIAGONAL ){
    return IntRect(opposite, release);
  }
  else if ( dir == HandleDirection::UP_DOWN ){
    return IntRect( IntPoint(0, release.y), IntPoint( imageSize.w - 1, opposite.y ) );
  }
  else if ( dir == HandleDirection::LEFT_RIGHT ){
    return IntRect( IntPoint(release.x, 0), IntPoint(opposite.x, imageSize.h - 1) );
  }
  assert(false);
  return IntRect();
}

static IntSize get_rescale_size( const CanvasResizeHandle& handle, const IntPoint& release ){
  HandleDirection dir = handle.GetDirection();
  IntPoint opposite = handle.Opposite().GetPos();
  IntSize imageSize = handle.GetCanvasSize();

  if ( dir == HandleDirection::DIAGONAL ){
    return IntRect(release, opposite).GetSize();
  }
  else if ( dir == HandleDirection::UP_DOWN ){
    return IntSize(imageSize.w, abs(release.y - opposite.y) );
  }
  else if ( dir == HandleDirection::LEFT_RIGHT ){
    return IntSize( abs( release.x - opposite.x ), imageSize.h );
  }
  assert( false );
  return IntSize(0,0);
}

static bool nearest_neighbour( int modifiers ){
  return fl(TOOLMODIFIER1, modifiers );
}

static void set_rescale_status( StatusInterface& status, const CanvasResizeHandle& handle, const IntPoint& m_release, int modifiers ){
  HandleDirection dir = handle.GetDirection();
  IntSize imageSize = handle.GetCanvasSize();

  if ( constrain_resize( modifiers, dir ) && nearest_neighbour(modifiers) ){
    status.SetMainText( "Scale nearest proportional." );
  }
  else if ( nearest_neighbour(modifiers) ) {
    status.SetMainText(can_constrain(dir) ? "Scale nearest. Shift=Constrain" :
      "Scale nearest");
  }
  else if ( constrain_resize(modifiers, dir) ){
    status.SetMainText( "Scale bilinear proportional. Ctrl=Nearest neighbour");
  }
  else {
    status.SetMainText( can_constrain(dir) ? "Shift=Proportional Ctrl=Nearest neighbour" :
      "Ctrl=Nearest Neighbour");
  }

  IntRect r(m_release, handle.Opposite().GetPos());
  if ( constrain_resize( modifiers, dir ) ){
    status.SetText( str_percentage( r.w , imageSize.w ) );
  }
  else {
    if ( dir == HandleDirection::UP_DOWN ){
      status.SetText( str_percentage(r.h, imageSize.h ));
    }
    else if ( dir == HandleDirection::LEFT_RIGHT ){
      status.SetText( str_percentage(r.w, imageSize.w ));
    }
    else {
      status.SetText( comma_sep(str_percentage(r.w, imageSize.w), str_percentage(r.h, imageSize.h)) );
    }
  }
}

void set_resize_status( StatusInterface& status, const CanvasResizeHandle& handle, const IntPoint& pos, int modifiers){
  HandleDirection dir = handle.GetDirection();
  IntSize imageSize = handle.GetCanvasSize();

  if ( constrain_resize( modifiers, dir ) ){
    status.SetMainText("Proportional resize");
  }
  else {
    status.SetMainText(can_constrain(dir) ? "Shift=Proportional" : "");
  }

  IntPoint opposite(handle.Opposite().GetPos());
  if ( dir == HandleDirection::LEFT_RIGHT ){
    status.SetText( space_sep(lbl("x", pos.x),
      lbl("w", abs(pos.x - opposite.x) + 1))); // Fixme: + 1 not always correct!
  }
  else if ( dir == HandleDirection::UP_DOWN ){
    status.SetText( space_sep(lbl("y", pos.y),
      lbl("h", abs(pos.y - opposite.y) + 1))); // Fixme: + 1 not always correct!
  }
  else {
    IntRect r( opposite, pos );
    status.SetText(space_sep(lbl("Pos", str(pos)), lbl("Size", str(r.GetSize()))));
  }
}

ResizeCanvas::ResizeCanvas( const CanvasResizeHandle& handle, Operation op )
  : Tool(ToolId::OTHER),
    m_handle(handle),
    m_release( handle.GetPos() ),
    m_operation(op),
    m_quality( ScaleQuality::BILINEAR )
{
  m_settings.Set(ts_BgCol, faint::DrawSource(faint::color_white()));
}

Command* ResizeCanvas::CreateCommand(const CursorPositionInfo& info){
  if ( m_operation == Resize ){
    IntRect rect(get_resize_rect(m_handle, m_release));
    return get_resize_command(info.canvas->GetBitmap(), rect, m_settings.Get(ts_BgCol));
  }
  else {
    assert(m_operation == Rescale );
    return get_rescale_command(get_rescale_size(m_handle, m_release),
      m_quality);
  }
}

void ResizeCanvas::Draw( FaintDC&, Overlays& overlays, const Point& ){
  Settings s( default_rectangle_settings() );
  HandleDirection dir = m_handle.GetDirection();
  IntPoint opposite(m_handle.Opposite().GetPos());
  if ( dir == HandleDirection::DIAGONAL ){
    overlays.Rectangle( floated( IntRect( opposite, m_release ) ) );
  }
  else if ( dir == HandleDirection::LEFT_RIGHT ){
    faint::coord x = floated(m_release.x) + ( m_release.x > opposite.x ?
      LITCRD(1.0) : LITCRD(0.0) ); // Offset to the right of the pixel if adjusting from the right
    overlays.VerticalLine( x );
  }
  else if ( dir == HandleDirection::UP_DOWN ){
    faint::coord y = floated(m_release.y) + ( m_release.y > opposite.y ?
      LITCRD(1.0) : LITCRD(0.0)); // Offset to the bottom of the pixel if adjusting from the bottom
    overlays.HorizontalLine(y);
  }
}

bool ResizeCanvas::DrawBeforeZoom(Layer) const{
  return false;
}

Command* ResizeCanvas::GetCommand(){
  return m_command.Retrieve();
}

Cursor ResizeCanvas::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::CROSSHAIR;
}

IntRect ResizeCanvas::GetRefreshRect( const IntRect& r, const Point& ) const{
  HandleDirection dir = m_handle.GetDirection();
  IntPoint opposite(m_handle.Opposite().GetPos());
  if ( dir == HandleDirection::DIAGONAL ){
    return IntRect( m_release, opposite );
  }
  else if ( dir == HandleDirection::LEFT_RIGHT ){
    return IntRect( IntPoint(m_release.x - 1, r.y - 1), IntSize(3, r.h + 1) );
  }
  else if ( dir == HandleDirection::UP_DOWN ){
    return IntRect( IntPoint( r.x, m_release.y - 1), IntSize(r.w, 3) );
  }
  else {
    assert( false );
    return IntRect( IntPoint(0, 0), IntSize(1, 1) );
  }
}

ToolResult ResizeCanvas::LeftDown( const CursorPositionInfo& ){
  return TOOL_NONE;
}

ToolResult ResizeCanvas::LeftUp( const CursorPositionInfo& info ){
  Point pos = info.pos;
  info.status->SetMainText("");
  info.status->SetText("");
  HandleDirection dir(m_handle.GetDirection());
  IntPoint opposite(m_handle.Opposite().GetPos());
  if ( constrain_resize(info.modifiers, dir) ) {
    pos = constrain_proportional( pos, floated(opposite), floated(m_handle.GetPos()) );
  }
  m_release = floored(pos);
  m_command.Set(CreateCommand(info));
  return TOOL_CHANGE;
}

ToolResult ResizeCanvas::Motion( const CursorPositionInfo& info ){
  Point pos = info.pos;
  IntPoint opposite(m_handle.Opposite().GetPos());
  if ( constrain_resize(info.modifiers, m_handle.GetDirection()) ){
    pos = constrain_proportional( pos, floated(opposite), floated(m_handle.GetPos()) );
  }

  m_quality = nearest_neighbour(info.modifiers) ? ScaleQuality::NEAREST :
    ScaleQuality::BILINEAR;

  m_release = floored(pos);

  if ( m_operation == Rescale ){
    set_rescale_status( *info.status, m_handle, m_release, info.modifiers );
  }
  else {
    set_resize_status( *info.status, m_handle, m_release, info.modifiers );
  }
  return TOOL_DRAW;
}

ToolResult ResizeCanvas::Preempt( const CursorPositionInfo& ){
  return TOOL_CHANGE;
}
