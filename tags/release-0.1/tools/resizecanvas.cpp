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

#include "resizecanvas.hh"
#include "commands/resizecommand.hh"
#include "cursors.hh"
#include "faintdc.hh"
#include "getappcontext.hh"
#include "objects/objrectangle.hh" // For GetRectangleSettings
#include <string>
#include <sstream>

bool Constrain( int modifiers ){
  return fl(TOOLMODIFIER2, modifiers);
}

bool NearestNeighbour( int modifiers ){
  return fl(TOOLMODIFIER1, modifiers );
}

void SetResizeStatus( StatusInterface& status, ResizeCanvas::Direction dir, const IntPoint& pos, const IntPoint& opposite, int modifiers){
  status.SetMainText( Constrain(modifiers)? "Proportional resize" : "Shift=Proportional");

  std::stringstream ss;
  if ( dir == ResizeCanvas::LEFT_RIGHT ){
    ss << "x: " << pos.x << " w: " << abs(pos.x - opposite.x) + 1;
  }
  else if ( dir == ResizeCanvas::UP_DOWN ){
    ss << "y: " << pos.y << " h: " << abs(pos.y - opposite.y) + 1;
  }
  else {
    IntRect r( opposite, pos );
    ss << "Pos: (" << int(pos.x) << "," << int(pos.y) << ") Size: (" << r.w << "," << r.h << ")";
  }
  status.SetText( ss.str() );
}

void SetRescaleStatus( StatusInterface& status, const IntPoint& pos, const IntPoint& opposite, const IntSize& imageSize, int modifiers ){
  if ( Constrain( modifiers ) && NearestNeighbour(modifiers) ){
    status.SetMainText( "Scale nearest proportional." );
  }
  else if ( NearestNeighbour(modifiers )) {
    status.SetMainText( "Scale nearest. Shift=Constrain" );
  }
  else if ( Constrain(modifiers ) ){
    status.SetMainText( "Scale bilinear proportional. Ctrl=Nearest neighbour");
  }
  else {
    status.SetMainText( "Shift=Proportional Ctrl=Nearest neighbour" );
  }

  if ( Constrain( modifiers ) ){
    status.SetText( Percentage( IntRect( pos, opposite ).w , imageSize.w ) );
  }
  else {
    status.SetText( Percentage( IntRect( pos, opposite ).w, imageSize.w ) +  "," +
      Percentage( IntRect( pos, opposite ).h, imageSize.h ) );
  }
}

ResizeCanvas::ResizeCanvas( ToolBehavior* prevTool, const IntPoint& handlePt, const IntPoint& oppositePt, const IntSize& imageSize, Operation op, Direction dir )
  : ToolBehavior(T_OTHER),
    m_imageSize( imageSize ),
    m_opposite( oppositePt ),
    m_release( handlePt ),
    m_prevTool( prevTool ),
    m_operation(op),
    m_direction( dir ),
    m_quality( RescaleCommand::Bilinear )
{}

ToolRefresh ResizeCanvas::LeftDown( const CursorPositionInfo&, int ){
  return TOOL_NONE;
}

bool ResizeCanvas::DrawBeforeZoom(Layer) const{
  return false;
}

ToolRefresh ResizeCanvas::LeftUp( const CursorPositionInfo& pInfo, int modifiers ){
  Point pos = pInfo.pos;
  StatusInterface& status = GetAppContext().GetStatusInfo();
  status.SetMainText("");
  status.SetText("");
  if ( Constrain(modifiers ) && m_direction == DIAGONAL ) {
    ConstrainProportional( pos, floated(m_imageSize) );
  }
  m_release = truncated(pos);
  return TOOL_CHANGE;
}

ToolRefresh ResizeCanvas::Motion( const CursorPositionInfo& info, int modifiers ){
  Point pos = info.pos;

  if ( Constrain(modifiers) && m_direction == DIAGONAL ){
    ConstrainProportional( pos, floated(m_imageSize));
  }

  m_quality = NearestNeighbour(modifiers) ? RescaleCommand::NearestNeighbour :
    RescaleCommand::Bilinear;

  m_release = truncated(pos);

  if ( m_operation == Rescale ){
    SetRescaleStatus( GetAppContext().GetStatusInfo(),
      m_release, m_opposite, m_imageSize, modifiers );
  }
  else {
    SetResizeStatus( GetAppContext().GetStatusInfo(), m_direction, m_release, m_opposite, modifiers );
  }
  return TOOL_OVERLAY;
}

bool ResizeCanvas::Draw( FaintDC&, Overlays& overlays, const Point& ){
  FaintSettings s = GetRectangleSettings();
  if ( m_direction == DIAGONAL ){
    overlays.Rectangle( floated( IntRect( m_opposite, m_release ) ) );
  }
  else if ( m_direction == LEFT_RIGHT ){
    float x = static_cast<float>(m_release.x) + ( m_release.x > m_opposite.x ?
      1.0f : 0.0f ); // Offset to the right of the pixel if adjusting from the right
    overlays.VerticalLine( x );
  }
  else if ( m_direction == UP_DOWN ){
    float y = static_cast<float>(m_release.y) + ( m_release.y > m_opposite.y ?
      1.0f : 0.0f); // Offset to the bottom of the pixel if adjusting from the bottom
    overlays.HorizontalLine(y);
  }
  return true;
}

IntRect ResizeCanvas::GetRefreshRect( const IntRect& r, const Point& ){
  if ( m_direction == DIAGONAL ){
    return IntRect( m_release, m_opposite );
  }
  else if ( m_direction == LEFT_RIGHT ){
    return IntRect( IntPoint(m_release.x - 1, r.y - 1), IntSize(3, r.h + 1) );
  }
  else if ( m_direction == UP_DOWN ){
    return IntRect( IntPoint( r.x, m_release.y - 1), IntSize(r.w, 3) );
  }
  else {
    assert( false );
    return IntRect( IntPoint(0, 0), IntSize(1, 1) );
  }
}

Command* ResizeCanvas::GetCommand(){
  if ( m_operation == Resize ){
    faint::Color bgCol = GetAppContext().GetToolSettings().Get( ts_BgCol );
    if ( m_direction == DIAGONAL ){
      return new ResizeCommand( IntRect(m_opposite, m_release), bgCol );
    }
    else if ( m_direction == UP_DOWN ){
      return new ResizeCommand( IntRect( IntPoint(0, m_release.y), IntPoint(m_imageSize.w - 1, m_opposite.y ) ), bgCol );
    }
    else if ( m_direction == LEFT_RIGHT ){
      return new ResizeCommand( IntRect( IntPoint(m_release.x, 0), IntPoint(m_opposite.x, m_imageSize.h - 1 ) ), bgCol );
    }
    else {
      assert( false );
      return 0;
    }
  }
  else {
    if ( m_direction == DIAGONAL ){
      IntRect r( m_release, m_opposite );
      return new RescaleCommand( r.GetSize(), m_quality );
    }
    else if ( m_direction == UP_DOWN ){
      return new RescaleCommand( IntSize( m_opposite.x * 2, abs( m_release.y - m_opposite.y ) ),
        m_quality );
    }
    else if ( m_direction == LEFT_RIGHT ){
      return new RescaleCommand( IntSize( abs( m_release.x - m_opposite.x ), m_opposite.y * 2 ),
        m_quality );
    }
    else {
      assert( false );
      return 0;
    }
  }
}

ToolBehavior* ResizeCanvas::GetNewTool(){
  return m_prevTool;
}

int ResizeCanvas::GetCursor( const CursorPositionInfo& ){
  return CURSOR_CROSSHAIR;
}

ToolRefresh ResizeCanvas::Preempt(){
  return TOOL_CHANGE;
}
