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

#include "rotateobject.hh"
#include "commands/tricommand.hh"
#include "commands/addobjectcommand.hh"
#include "util/angle.hh"
#include "getappcontext.hh"
#include "faintdc.hh"

ObjRotateBehavior::ObjRotateBehavior( Object* obj, int handleIndex, ToolBehavior* previous, bool copy )
  : ToolBehavior(T_OBJ_SEL),
    m_object(obj),
    m_oldTri( obj->GetTri() ),
    m_previousTool( previous ),
    m_active(true),
    m_copy( copy )
{  
  if ( m_copy ){
    m_object = m_object->Clone();
  }
  m_handle = m_object->GetResizePoints()[handleIndex];
  Rect objRect = BoundingRect( m_oldTri );
  if ( handleIndex == 0 ){
    m_pivot = objRect.BottomRight();
  }
  else if ( handleIndex == 1 ){
    m_pivot = objRect.BottomLeft();
  }
  else if ( handleIndex == 2 ){
    m_pivot = objRect.TopRight();
  }
  else if ( handleIndex == 3 ){
    m_pivot = objRect.TopLeft();
  }
  else {
    assert( false );
  }

  m_object->SetActive();
}

bool ObjRotateBehavior::DrawBeforeZoom(Layer) const{
  return false;
}

ToolRefresh ObjRotateBehavior::LeftDown( const CursorPositionInfo&, int ){
  return TOOL_NONE;
}

ToolRefresh ObjRotateBehavior::LeftUp( const CursorPositionInfo&, int ){
  m_active = false;
  m_object->ClearActive();
  return TOOL_CHANGE;
}

ToolRefresh ObjRotateBehavior::Motion( const CursorPositionInfo& posInfo, int modifiers ){
  if ( !m_active ){
    return TOOL_NONE;
  }
  const Point& pos = posInfo.pos;
  faint::radian a1 = rad_angle( m_pivot.x, m_pivot.y, m_handle.x, m_handle.y );
  faint::radian a2 = rad_angle( m_pivot.x, m_pivot.y, pos.x, pos.y );
  faint::coord delta = a2 - a1;

  if ( fl(TOOLMODIFIER2, modifiers ) ){
    if ( delta > - faint::pi / 4.0 && delta < faint::pi / 4.0 ){
      delta = 0;
    }
    else if ( delta > faint::pi / 4.0 && delta < ( faint::pi * 3.0 / 4.0 ) ){
      delta = faint::pi / 2;
    }
    else if ( delta > (faint::pi * 3/4.0 ) && delta < ( faint::pi * 3.0 / 4.0 ) + faint::pi / 4.0 ){
      delta = faint::pi;
    }
    else {
      delta = faint::pi + faint::pi / 2.0;
    }
  }

  StatusInterface& status = GetAppContext().GetStatusInfo();
  status.SetText( StrAngle( delta * faint::degreePerRadian ) );

  Tri t2( Rotated( m_oldTri, delta, m_pivot ) );
  m_object->SetTri( t2 );
  return TOOL_OVERLAY;
}

bool ObjRotateBehavior::Draw( FaintDC& dc, Overlays& overlays, const Point& ){
  if ( m_copy ){
    m_object->Draw( dc );
  }
  overlays.Pivot( m_pivot );
  return true;
}

IntRect ObjRotateBehavior::GetRefreshRect( const IntRect&, const Point& ){
  return m_object->GetRefreshRect();
}

Command* ObjRotateBehavior::GetCommand(){
  if ( m_copy ){
    return new AddObjectCommand( m_object, true );
  } 
  else {
    return new TriCommand( m_object, m_object->GetTri(), m_oldTri );
  }
}

int ObjRotateBehavior::GetCursor( const CursorPositionInfo& ){
  return CURSOR_ROTATE_RIGHT;
}

ToolBehavior* ObjRotateBehavior::GetNewTool(){
  return m_previousTool;
}

ToolRefresh ObjRotateBehavior::Preempt(){
  return TOOL_NONE;
}
