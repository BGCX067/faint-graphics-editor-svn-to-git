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

#include "moveobject.hh"
#include "commands/commandbunch.hh"
#include "commands/addobjectcommand.hh"
#include "commands/tricommand.hh"
#include "util/util.hh"
#include "util/objutil.hh"
#include "util/commandutil.hh"
#include <algorithm> // std::find

ObjMoveBehavior::ObjMoveBehavior( CanvasInterface* canvas, Object* mainObject, std::vector<Object*> allObjects, ToolBehavior* other, const Point& offset, bool copy )
  : ToolBehavior(T_OBJ_SEL), // Identify as the Object selection tool
    m_active(true),
    m_copy( copy ),
    m_offset( offset ),
    m_prevTool( other ),
    m_object( mainObject ),
    m_oldTri( mainObject->GetTri() ),
    m_boundingBox( mainObject->GetRect() ),
    m_refreshRect( floated( mainObject->GetRefreshRect() ) )
{
  for ( size_t i = 0; i != allObjects.size(); i++ ){
    Object* object = allObjects[i];
    m_boundingBox = Union( m_boundingBox, object->GetRect() );
    m_refreshRect = Union( m_refreshRect, floated(object->GetRefreshRect()) );
    const Tri tri(object->GetTri());
    if ( m_copy ){
      canvas->DeselectObject( object );
      Object* objCopy = object->Clone();
      m_objects.push_back( objCopy );
      m_origTris.push_back( tri );
      if ( object == m_object ){
        m_object = objCopy;
      }
    }
    else {
      m_objects.push_back( object );
      m_origTris.push_back( tri );
    }
  }

  for ( size_t i = 0; i != m_objects.size(); i++ ){
    m_objects[i]->SetActive();
  }
}

bool ObjMoveBehavior::DrawBeforeZoom(Layer) const{
  return false;
}

Object* ObjMoveBehavior::GetMainObject(){
  return m_object;
}

ToolRefresh ObjMoveBehavior::LeftDown( const CursorPositionInfo&, int ){
  return TOOL_NONE;
}

ToolRefresh ObjMoveBehavior::LeftUp( const CursorPositionInfo& info, int modifiers ){
  m_active = false;
  for ( size_t i = 0; i!= m_objects.size(); i++ ){
    m_objects[i]->ClearActive();
  }

  m_pos = info.pos - m_offset;

  if ( fl(TOOLMODIFIER1, modifiers ) ){
    // std::vector<Object*> objects = info.canvas->GetObjects();
    // Point delta = SnapObject( info.canvas->GetObjects(), info.canvas->GetGrid() );
    // m_pos += delta;
  }

  if ( (modifiers & TOOLMODIFIER2) == TOOLMODIFIER2 ){
    ConstrainPos( m_pos, m_oldTri.P0() );
  }
  return TOOL_CHANGE;
}

ToolRefresh ObjMoveBehavior::Motion( const CursorPositionInfo& info, int modifiers ){
  if ( !m_active ){
    return TOOL_NONE;
  }
  m_pos = info.pos - m_offset;
  if ( fl( TOOLMODIFIER2, modifiers ) ){
    ConstrainPos( m_pos, m_oldTri.P0() );
  }
  Point delta = Delta( m_object, m_pos );
  Offset( m_objects, delta );
  m_boundingBox = Translated( m_boundingBox, delta );
  m_refreshRect = Translated( m_refreshRect, delta );

  if ( fl( TOOLMODIFIER1, modifiers ) ){
    Point delta2 = SnapObject( info.canvas->GetObjects(), info.canvas->GetGrid() );
    Offset( m_objects, delta2 );
    m_boundingBox = Translated( m_boundingBox, delta2 );
    m_refreshRect = Translated( m_refreshRect, delta2 );
  }
  return TOOL_OVERLAY;
}

Point ObjMoveBehavior::SnapObject( std::vector<Object*> objects, const Grid& grid ){
  std::vector<Point> points = m_object->GetAttachPoints();
  if ( points.empty() ){
    return Point(0,0);
  }
  Point p_first(points.front());
  for ( size_t i = 0; i != m_objects.size(); i++ ){
    std::vector<Object*>::iterator it = std::find( objects.begin(), objects.end(), m_objects[i] );
    if ( it != objects.end() ){
      objects.erase(it);
    }
  }

  Point p_adj = Snap( p_first, objects, grid );
  Point delta = p_adj - p_first;
  return delta;
}

bool ObjMoveBehavior::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( m_copy && m_active ){
    for ( size_t i = 0; i != m_objects.size(); i++ ){
      m_objects[i]->Draw( dc );
    }
  }
  return true;
}

IntRect ObjMoveBehavior::GetRefreshRect( const IntRect&, const Point& ){
  return truncated(m_refreshRect);
}

Command* ObjMoveBehavior::GetCommand(){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != m_objects.size(); i++ ){
    Object* obj( m_objects[i] );
    const Tri& oldTri( m_origTris[i] );
    if ( m_copy ){
      commands.push_back( new AddObjectCommand( obj, true ) );
    }
    else {
      commands.push_back( new TriCommand( obj, obj->GetTri(), oldTri ) );
    }
  }
  return PerhapsBunch( CMD_TYPE_OBJECT, commands );
}

ToolBehavior* ObjMoveBehavior::GetNewTool(){
  return m_prevTool;
}

int ObjMoveBehavior::GetCursor( const CursorPositionInfo& ){
  return m_copy ? CURSOR_CLONE : CURSOR_MOVE;
}

ToolRefresh ObjMoveBehavior::Preempt(){
  return TOOL_CHANGE;
}

