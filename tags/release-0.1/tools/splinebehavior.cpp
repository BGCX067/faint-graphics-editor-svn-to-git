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

#include "splinebehavior.hh"
#include "commands/addobjectcommand.hh"
#include "cursors.hh"
#include "faintdc.hh"
#include "getappcontext.hh"

#include "objects/objspline.hh"
#include "settingid.hh"
using std::min;
using std::max;

SplineCommand::SplineCommand( const Points& points, const FaintSettings& settings )
  : Command( CMD_TYPE_RASTER ),
    m_points( points ),
    m_settings( settings )
{}

void SplineCommand::Do( faint::Image& img ){
  FaintDC dc( img.GetBitmapRef() );
  dc.Spline( m_points.GetPointsDumb(), m_settings );
}


SplineBehavior::SplineBehavior()
  : ToolBehavior(T_SPLINE),
    m_otherButton(RIGHT_MOUSE)
{
  m_active = false;
  m_settings.Set( ts_LineWidth, 1 );
  m_settings.Set( ts_LineCap, 1 );
  m_settings.Set( ts_LineStyle, 1 );
  m_settings.Set( ts_FgCol, faint::Color(0,0,0) );
  m_settings.Set( ts_BgCol, faint::Color(0,0,0) );
}

bool SplineBehavior::DrawBeforeZoom(Layer layer) const{
  return layer == LAYER_RASTER;
}

ToolRefresh SplineBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  if ( m_active && (modifiers & m_otherButton) == m_otherButton ) {
    m_points.Append( info.pos );
    if ( info.layerType == LAYER_RASTER ) {
      m_command.Set( new SplineCommand( m_points, m_settings ) );
    }
    else {
      m_command.Set( new AddObjectCommand( new ObjSpline( m_points, m_settings ) ) );
    }
    m_points.Clear();
    m_active = false;
    return TOOL_COMMIT;
  }
  else if ( !m_active ){
    m_active = true;
    m_p1 = info.pos;
    m_p2 = info.pos;
    m_points.Append( info.pos );

    if ( RIGHT_MOUSE == mbtn( modifiers ) ) {
      m_settings.Set( ts_SwapColors, true );
      m_otherButton = LEFT_MOUSE;
    }
    else {
      m_settings.Set( ts_SwapColors, false );
      m_otherButton = RIGHT_MOUSE;
    }
  }
  m_points.Append( info.pos );
  m_p1 = MinCoords(m_p1, info.pos);
  m_p2 = MaxCoords(m_p2, info.pos);
  return TOOL_NONE;
}

ToolRefresh SplineBehavior::LeftUp( const CursorPositionInfo&, int ){
  return TOOL_NONE;
}

ToolRefresh SplineBehavior::Motion( const CursorPositionInfo& info, int ){
  StatusInterface& status = GetAppContext().GetStatusInfo();
  status.SetText( StrPoint( info.pos ), 0 );
  if ( m_active ){
    m_points.AdjustBack( info.pos );
    StrBtn btn( m_otherButton );

    status.SetMainText( std::string( btn.Other( true ) + " click to add points, " + btn.This(false) + " click to stop." ) );
    return TOOL_OVERLAY;
  }
  else {
    status.SetMainText( "Click to start drawing a spline" );
  }
  return TOOL_NONE;
}


ToolRefresh SplineBehavior::Preempt(){
  if ( m_active ){
    m_active = false;
    m_points.Clear();
    return TOOL_CANCEL;
  }
  return TOOL_NONE;
}

bool SplineBehavior::Draw( FaintDC& dc, Overlays&, const Point& currPos ){
  if ( m_active ){
    m_points.Append( currPos );
    dc.Spline( m_points.GetPointsDumb(), m_settings );
    m_points.PopBack();
    return true;
  }
  return false;
}

IntRect SplineBehavior::GetRefreshRect( const IntRect&, const Point& currPos ){
  return truncated( Inflated( BoundingRect( m_p1, m_p2, currPos ), m_settings.Get( ts_LineWidth ) ) );
}

int SplineBehavior::GetCursor( const CursorPositionInfo& ){
  return CURSOR_CROSSHAIR;
}

Command* SplineBehavior::GetCommand(){
  return m_command.Retrieve();
}

unsigned int SplineBehavior::GetStatusFieldCount(){
  return 1;
}
