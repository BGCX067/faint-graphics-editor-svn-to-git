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

#include "objects/objspline.hh"
#include "rendering/faintdc.hh"
#include "tools/spline-tool.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

SplineTool::SplineTool()
  : Tool(ToolId::SPLINE),
    m_otherButton(RIGHT_MOUSE)
{
  m_active = false;
  m_settings = default_spline_settings();
}

bool SplineTool::AllowsGlobalRedo() const{
  return !m_active;
}

bool SplineTool::CanRedo() const{
  return m_active && m_states.CanRedo();
}

bool SplineTool::CanUndo() const{
  return m_active && m_states.CanUndo();

}

ToolResult SplineTool::Commit(Layer layerType){
  m_command.Set( add_or_draw(new ObjSpline(m_points, m_settings), layerType) );
  m_states.Clear();
  m_points.Clear();
  m_active = false;
  return TOOL_COMMIT;
}

void SplineTool::Draw( FaintDC& dc, Overlays&, const Point& currPos ){
  if ( m_active ){
    m_points.Append( currPos );
    dc.Spline( m_points.GetPointsDumb(), m_settings );
    m_points.PopBack();
  }
}

bool SplineTool::DrawBeforeZoom(Layer layer) const{
  return layer == Layer::RASTER;
}

Command* SplineTool::GetCommand(){
  return m_command.Retrieve();
}

Cursor SplineTool::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::CROSSHAIR;
}

std::string SplineTool::GetRedoName() const{
  return "Add Point";
}

IntRect SplineTool::GetRefreshRect( const IntRect&, const Point& currPos ) const{
  return floored( inflated( bounding_rect( m_p1, m_p2, currPos ), m_settings.Get( ts_LineWidth ) ) );
}

std::string SplineTool::GetUndoName() const{
  return "Add Point";
}

ToolResult SplineTool::LeftDown( const CursorPositionInfo& info ){
  if ( m_active && fl(m_otherButton, info.modifiers) ){
    m_points.Append( info.pos );
    return Commit(info.layerType);
  }
  else if ( !m_active ){
    m_active = true;
    m_p1 = info.pos;
    m_p2 = info.pos;
    m_points.Append( info.pos );

    if ( RIGHT_MOUSE == mbtn( info.modifiers ) ) {
      m_settings.Set( ts_SwapColors, true );
      m_otherButton = LEFT_MOUSE;
    }
    else {
      m_settings.Set( ts_SwapColors, false );
      m_otherButton = RIGHT_MOUSE;
    }
  }
  m_points.Append( info.pos );
  m_states.Did( info.pos );
  m_p1 = min_coords(m_p1, info.pos);
  m_p2 = max_coords(m_p2, info.pos);
  return TOOL_NONE;
}

ToolResult SplineTool::LeftUp( const CursorPositionInfo& ){
  return TOOL_NONE;
}

ToolResult SplineTool::Motion( const CursorPositionInfo& info ){
  info.status->SetText( str( info.pos ), 0 );
  if ( m_active ){
    m_points.AdjustBack( info.pos );
    StrBtn btn( m_otherButton );
    info.status->SetMainText( space_sep(btn.Other(true), "click to add points,",
	btn.This(false), "click to stop.") );
    return TOOL_DRAW;
  }
  else {
    info.status->SetMainText( "Click to start drawing a spline" );
  }
  return TOOL_NONE;
}

ToolResult SplineTool::Preempt( const CursorPositionInfo& info ){
  if ( m_active ){
    return Commit(info.layerType);
  }
  return TOOL_NONE;
}

void SplineTool::Redo(){
  m_points.Append(m_states.Redo());
}

void SplineTool::Undo(){
  m_states.Undo();
  m_points.PopBack();
  if ( !m_states.CanUndo() ){
    m_active = false;
    m_states.Clear();
    m_points.Clear();
  }
}
