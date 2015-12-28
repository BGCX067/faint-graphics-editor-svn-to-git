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

#include "commands/tricommand.hh"
#include "geo/grid.hh"
#include "objects/object.hh"
#include "tasks/move-object.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"
#include "util/util.hh"

MoveObjectTask::MoveObjectTask( Object* mainObject, const objects_t& allObjects, const Point& offset, MoveMode::Type mode )
  : m_copy(mode == MoveMode::COPY),
    m_fullRefresh(mode == MoveMode::COPY),
    m_mainObject(mainObject),
    m_offset(offset),
    m_oldTri(mainObject->GetTri()),
    m_boundingBox(mainObject->GetRect()),
    m_refreshRect(floated(mainObject->GetRefreshRect() ))
{
  for ( size_t i = 0; i != allObjects.size(); i++ ){
    Object* object = allObjects[i];
    m_boundingBox = union_of( m_boundingBox, object->GetRect() );
    m_refreshRect = union_of( m_refreshRect, floated(object->GetRefreshRect()) );
    const Tri tri(object->GetTri());
    m_objects.push_back(object);
    m_origTris.push_back(object->GetTri());
    object->SetActive();
  }

  if ( m_copy ){
    // Set the pending command for copied objects so that the objects
    // are freed if the command is left unretrieved.
    m_command.Set(get_add_objects_command( m_objects, select_added(true), "Clone" ));
  }

  m_refreshRect = inflated(m_refreshRect, 5, 5);
}

TaskResult MoveObjectTask::Commit(){
  if ( m_copy ){
    // When copying, the command is prepared by the constructor.
    return TASK_COMMIT_AND_CHANGE;
  }
  m_command.Set(get_move_objects_command(m_objects, New(get_tris(m_objects)), Old(m_origTris)));
  return TASK_COMMIT_AND_CHANGE;
}

bool MoveObjectTask::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( m_copy ){
    // Copied objects need to be drawn here, as they're not added to the canvas yet.
    // Moved objects will be drawn by the canvas.
    for ( size_t i = 0; i != m_objects.size(); i++ ){
      m_objects[i]->Draw(dc);
    }
  }
  // Claim drawing took place regardless of copying so that the
  // refresh-rect is redrawn.
  return true;
}

bool MoveObjectTask::DrawBeforeZoom(Layer::type) const{
  return false;
}

Command* MoveObjectTask::GetCommand(){
  return m_command.Retrieve();
}

Cursor::type MoveObjectTask::GetCursor( const CursorPositionInfo& ) const{
  return m_copy ? Cursor::CLONE : Cursor::MOVE;
}

Task* MoveObjectTask::GetNewTask(){
  return 0;
}

IntRect MoveObjectTask::GetRefreshRect( const IntRect&, const Point& ) const{
  return truncated(m_refreshRect); // Fixme
}

TaskResult MoveObjectTask::LeftDown( const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult MoveObjectTask::LeftUp( const CursorPositionInfo& info ){
  for ( size_t i = 0; i != m_objects.size(); i++ ){
    m_objects[i]->ClearActive();
  }
  m_pos = info.pos - m_offset;
  if ( fl(TOOLMODIFIER1, info.modifiers) ){
    // Fixme: What
    // objects_t objects = info.canvas->GetObjects();
    // Point delta = SnapObject( info.canvas->GetObjects(), info.canvas->GetGrid() );
    // m_pos += delta;
  }

  if ( fl(TOOLMODIFIER2, info.modifiers ) ){
    constrain_pos(m_pos, m_oldTri.P0());
  }
  return Commit();
}

TaskResult MoveObjectTask::Motion( const CursorPositionInfo& info ){
  m_pos = info.pos - m_offset;
  if ( fl(TOOLMODIFIER2, info.modifiers ) ){
    constrain_pos(m_pos, m_oldTri.P0());
  }
  Point delta = get_delta(m_mainObject, m_pos);
  offset_by(m_objects, delta);
  m_boundingBox = translated(m_boundingBox, delta);
  m_refreshRect = translated(m_refreshRect, delta);

  if ( fl(TOOLMODIFIER1, info.modifiers) ){
    Point delta2 = SnapObject(info.canvas->GetObjects(), info.canvas->GetGrid());
    offset_by(m_objects, delta2);
    m_boundingBox = translated(m_boundingBox, delta2);
    m_refreshRect = translated(m_refreshRect, delta2);
  }
  return TASK_DRAW;
}

TaskResult MoveObjectTask::Preempt( const CursorPositionInfo& ){
  return Commit();
}

Point MoveObjectTask::SnapObject( objects_t snappable, const Grid& grid ){
  std::vector<Point> points = m_mainObject->GetSnappingPoints();
  if ( points.empty() ){
    return Point(0,0);
  }
  // Do not snap to any of the moved objects
  remove(m_objects, from(snappable));
  Point p_first(points.front());
  Point p_adj = snap( p_first, snappable, grid );
  Point delta = p_adj - p_first;
  return delta;
}
