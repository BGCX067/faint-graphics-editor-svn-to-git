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

#include "commands/pointcommand.hh"
#include "commands/addobjectcommand.hh"
#include "movepoint.hh"
#include "getappcontext.hh"
#include "util/objutil.hh"
#include <algorithm>
#include "angle.hh"

MovePointBehavior::MovePointBehavior( Object* obj, size_t pointIndex, ToolBehavior* prevTool, bool copy )
  : ToolBehavior(T_OBJ_SEL), // Identify as the object selection tool
    m_object( obj ),
    m_pointIndex( pointIndex ),
    m_oldPos( m_object->GetPoint( pointIndex )),
    m_prevTool( prevTool ),
    m_copy(copy)
{
  m_active = true;
  if ( m_copy ){
    m_object = obj->Clone();
  }
  m_object->SetActive();

  const Point& other = ( m_pointIndex == 0 ?
    m_object->GetPoint( 1 ) :
    m_object->GetPoint( m_pointIndex - 1 ));

  faint::radian twoPi = 2 * faint::pi;
  m_oldAngle = -rad_angle( other.x, other.y, m_oldPos.x, m_oldPos.y );
  if ( m_oldAngle < 0 ){
    m_oldAngle = twoPi + m_oldAngle;
  }
}

bool MovePointBehavior::DrawBeforeZoom(Layer) const{
  return true;
}

ToolRefresh MovePointBehavior::LeftDown( const CursorPositionInfo&, int ){
  return TOOL_NONE;
}

ToolRefresh MovePointBehavior::LeftUp( const CursorPositionInfo&, int ){
  m_active = false;
  m_object->ClearActive();
  return TOOL_CHANGE;
}

ToolRefresh MovePointBehavior::Motion( const CursorPositionInfo& info, int modifiers ){
  if ( !m_active ){
    return TOOL_NONE;
  }

  Point p = info.pos;
  if ( modifiers & TOOLMODIFIER1 ){
    std::vector<Object*> objects = info.canvas->GetObjects();
    if ( !m_copy ){
      // Don't snap to yourself
      objects.erase( std::find( objects.begin(), objects.end(), m_object ) );
    }
    p = Snap( p, objects, info.canvas->GetGrid() );
  }
  else if ( modifiers & TOOLMODIFIER2 ){
    const Point& other = ( m_pointIndex == 0 ?
      m_object->GetPoint( 1 ) :
      m_object->GetPoint( m_pointIndex - 1 ));

    p = AdjustToDefault( other, p, faint::pi / 4, m_oldAngle  );
  }
  m_object->SetPoint( p, m_pointIndex );
  std::string status = m_object->StatusString();
  if ( !status.empty() ){
    GetAppContext().GetStatusInfo().SetText( status, 0 );
  }
  return TOOL_OVERLAY;
}

bool MovePointBehavior::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( m_copy && m_active ){
    m_object->Draw( dc );
  }
  return true;
}

IntRect MovePointBehavior::GetRefreshRect( const IntRect&, const Point& ){
  return m_object->GetRefreshRect();
}

Command* MovePointBehavior::GetCommand(){
  if ( m_copy ){
    return new AddObjectCommand( m_object, true );
  }
  else {
    return new PointCommand( m_object, m_pointIndex, m_object->GetPoint( m_pointIndex ), m_oldPos );
  }
}

int MovePointBehavior::GetCursor( const CursorPositionInfo& ){
  return CURSOR_ARROW;
}

ToolBehavior* MovePointBehavior::GetNewTool(){
  return m_prevTool;
}

ToolRefresh MovePointBehavior::Preempt(){
  return TOOL_NONE;
}
