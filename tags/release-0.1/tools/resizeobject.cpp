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

#include "resizeobject.hh"
#include "canvasinterface.hh"
#include "commands/tricommand.hh"
#include "commands/addobjectcommand.hh"
#include "cursors.hh"
#include "faintdc.hh"
#include "getappcontext.hh"
#include "objects/objrectangle.hh"
#include "objects/object.hh"
#include "util/objutil.hh"
#include "util/util.hh"
#include <algorithm>

ObjResizeBehavior::ObjResizeBehavior(Object* object, int handleIndex, ToolBehavior* other, bool copy )
  : ToolBehavior(T_OBJ_SEL),
    m_object( object ),
    m_lock_x(false),
    m_lock_y(false),
    m_oldTri( object->GetTri() ),
    m_p0( 0, 0 ),
    m_origin( 0, 0 ),
    m_prevTool( other ),    
    m_active(true),
    m_copy(copy)
{
  if ( m_copy ){
    m_object = m_object->Clone();
  }
  m_object->SetActive();
  Rect objRect = BoundingRect( m_oldTri );
  if ( handleIndex == 0 ){
    m_origin = objRect.BottomRight();
    m_p0 = objRect.TopLeft();
  }
  else if ( handleIndex == 1 ){
    m_origin = objRect.BottomLeft();
    m_p0 = objRect.TopRight();
  }
  else if ( handleIndex == 2 ){
    m_origin = objRect.TopRight();
    m_p0 = objRect.BottomLeft();
  }
  else if ( handleIndex == 3 ){
    m_origin = objRect.TopLeft();
    m_p0 = objRect.BottomRight();
  }
  else if ( handleIndex == 4 ){
    m_p0 = Point( objRect.Left(), objRect.Center().y );
    m_origin = Point( objRect.Right(), objRect.Center().y );
    m_lock_y = true;
  }
  else if ( handleIndex == 5 ){
    m_p0 = Point( objRect.Right(), objRect.Center().y );
    m_origin = Point( objRect.Left(), objRect.Center().y );
    m_lock_y = true;
  }
  else if ( handleIndex == 6 ){
    m_p0 = Point( objRect.Center().x , objRect.y );
    m_origin = Point( objRect.Center().x, objRect.Bottom() );
    m_lock_x = true;
  }
  else if ( handleIndex == 7 ){
    m_p0 = Point( objRect.Center().x, objRect.Bottom() );
    m_origin = Point( objRect.Center().x , objRect.y );
    m_lock_x = true;
  }
}

bool ObjResizeBehavior::DrawBeforeZoom(Layer) const{
  return false;
}

ToolRefresh ObjResizeBehavior::LeftDown( const CursorPositionInfo&, int ){
  // The ObjResizeBehavior is initialized in LeftDown in the
  // ObjSelectBehavior, and should not survive past LeftUp, so
  // LeftDown should never be called.
  return TOOL_NONE;
}

ToolRefresh ObjResizeBehavior::LeftUp( const CursorPositionInfo&, int ){
  m_object->ClearActive();
  m_active = false;
  return TOOL_CHANGE;
}

ToolRefresh ObjResizeBehavior::Motion( const CursorPositionInfo& info, int modifiers ){
  if ( !m_active ){
    return TOOL_NONE;
  }
  Point p1 = info.pos;
  if ( fl(TOOLMODIFIER1, modifiers) ){
    std::vector<Object*> objects = info.canvas->GetObjects();
    if ( !m_copy ){
      // Don't snap to yourself
      objects.erase( std::find( objects.begin(), objects.end(), m_object ) );
    }
    if ( m_lock_y ){
      Rect oldRect( BoundingRect( m_oldTri ) );
      p1.x = SnapX( p1.x, objects, info.canvas->GetGrid(), oldRect.Top(), oldRect.Bottom() );
    }
    else if ( m_lock_x ){
      Rect oldRect( BoundingRect( m_oldTri ) );
      p1.y = SnapY( p1.y, objects, info.canvas->GetGrid(), oldRect.Left(), oldRect.Right() );
    }
    else {
      p1 = Snap( p1, objects, info.canvas->GetGrid() );
    }
  }
  faint::coord scale_x = m_lock_x ? 1.0 : ( p1.x - m_origin.x ) / ( m_p0.x - m_origin.x );
  faint::coord scale_y = m_lock_y ? 1.0 : ( p1.y - m_origin.y ) / ( m_p0.y - m_origin.y );
  
  bool constrain = fl( TOOLMODIFIER2, modifiers );
  if ( constrain && !m_lock_x && !m_lock_y ) {
    scale_y = ( (scale_x < 0) == (scale_y < 0) ) ? scale_x : - scale_x;
  }

  m_object->SetTri( Scaled( m_oldTri, scale_x, scale_y, m_origin ) );
  StatusInterface& status = GetAppContext().GetStatusInfo();
  status.SetText( StrScale( scale_x, scale_y ) );
  return TOOL_OVERLAY;
}

bool ObjResizeBehavior::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( m_copy && m_active ){
    m_object->Draw( dc );
  }
  return true;
}

IntRect ObjResizeBehavior::GetRefreshRect( const IntRect&, const Point& ){
  return m_object->GetRefreshRect();
}

Command* ObjResizeBehavior::GetCommand(){
  if ( m_copy ){
    return new AddObjectCommand( m_object, true );
  }
  return new TriCommand( m_object, m_object->GetTri(), m_oldTri );
}

ToolBehavior* ObjResizeBehavior::GetNewTool(){
  return m_prevTool;
}

int ObjResizeBehavior::GetCursor( const CursorPositionInfo& ){
  return CURSOR_ARROW;
}

ToolRefresh ObjResizeBehavior::Preempt(){
  m_active = false;
  m_object->ClearActive();
  return TOOL_CHANGE;

}
