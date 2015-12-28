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

#include "penbehavior.hh"
#include "cursors.hh"
#include "faintdc.hh"
#include "getappcontext.hh"
#include "geo/geotypes.hh"
#include "settingid.hh"
#include "util/util.hh"

using std::min;
using std::max;

class PenCommand : public Command {
public:
  PenCommand( const std::vector<IntPoint>& points, const FaintSettings& settings )
    : Command( CMD_TYPE_RASTER ),
      m_settings( settings ),
      m_points( points )
  {}

  void Do( faint::Image& image ){
    FaintDC dc( image.GetBitmapRef() );
    dc.Lines( m_points, m_settings );
  }

private:
  FaintSettings m_settings;
  std::vector<IntPoint> m_points;
};

bool constrain_held( int modifiers ){
  return (modifiers & TOOLMODIFIER2 ) == TOOLMODIFIER2;
}

PenBehavior::PenBehavior()
  : ToolBehavior(T_PEN)
{
  m_active = false;
  m_constrainDir = constrain_dir::NONE;
  m_settings.Set( ts_FgCol, faint::Color( 0, 0, 0 ) );
  m_settings.Set( ts_BgCol, faint::Color( 0, 0, 0 ) );
  m_settings.Set( ts_SwapColors, false );
}

ToolRefresh PenBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  m_active = true;
  m_origin = truncated(info.pos);
  m_constrainDir = constrain_dir::NONE;
  m_settings.Set( ts_SwapColors, mbtn(modifiers) == RIGHT_MOUSE );
  m_points.clear();
  m_points.push_back( m_origin );
  return TOOL_OVERLAY;
}

bool PenBehavior::DrawBeforeZoom(Layer) const{
  return true;
}

ToolRefresh PenBehavior::LeftUp( const CursorPositionInfo&, int ){
  if ( !m_active ){
    return TOOL_NONE;
  }

  m_active = false;
  m_command.Set( new PenCommand( m_points, m_settings ) );
  m_points.clear();
  return TOOL_COMMIT;
}

ToolRefresh PenBehavior::Motion( const CursorPositionInfo& info,  int modifiers ){
  Point pos = info.pos;
  GetAppContext().GetStatusInfo().SetText( StrPoint( pos ), 0 );

  if ( !m_active ){
    m_constrainDir = constrain_dir::NONE;
    return TOOL_NONE;
  }

  if ( constrain_held( modifiers ) ) {
    if ( m_constrainDir == constrain_dir::NONE ){
      if ( distance( truncated(pos), m_origin ) > 5 ){
        // Lock the constrain direction after a certain distance
        // to avoid weird switches
        m_constrainDir = ConstrainPos( pos, floated(m_origin) );
      }
      else {
        ConstrainPos( pos, floated(m_origin) );
      }
    }
    else {
      ConstrainPos( pos, floated(m_origin), m_constrainDir );
    }
  }
  else {
    m_origin = truncated(info.pos);
    m_constrainDir = constrain_dir::NONE;
  }

  IntPoint iPos( truncated(pos) );

  if ( m_points[ m_points.size() - 1 ] != iPos ){
    m_points.push_back( iPos );
  }
  return TOOL_OVERLAY;
}

ToolRefresh PenBehavior::Preempt(){
  if ( !m_active ){
    return TOOL_NONE;
  }
  m_active = false;
  m_command.Set( new PenCommand( m_points, m_settings ) );
  m_points.clear();
  return TOOL_COMMIT;
}

bool PenBehavior::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( !m_active ){
    return false;
  }

  dc.Lines( m_points, m_settings );
  return true;
}

IntRect PenBehavior::GetRefreshRect( const IntRect&, const Point& ){
  if ( m_points.empty() ){
    return IntRect( IntPoint(0,0), IntSize(0,0) );
  }
  else if ( m_points.size() == 1 ){
    return IntRect( m_points[0], IntSize(1,1));
  }
  else {
    IntPoint p0 = m_points[ m_points.size() -1 ];
    IntPoint p1 = m_points[ m_points.size() -2 ];
    return IntRect( p0, p1 );
  }
}

int PenBehavior::GetCursor( const CursorPositionInfo& ){
  return CURSOR_PEN;
}

unsigned int PenBehavior::GetStatusFieldCount(){
  return 1;
}

Command* PenBehavior::GetCommand(){
  return m_command.Retrieve();
}
