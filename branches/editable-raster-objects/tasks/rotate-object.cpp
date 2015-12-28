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
#include "commands/add-object-cmd.hh"
#include "commands/tri-cmd.hh"
#include "gui/gui-constants.hh"
#include "objects/object.hh"
#include "rendering/overlay.hh"
#include "tasks/rotate-object.hh"
#include "util/angle.hh"
#include "util/flag.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"

// Returns the position of the opposite corner of the indicated handle
Point pivot_from_handle(const Object* obj, int handleIndex ){
  Tri tri(obj->GetTri());
  if ( object_aligned_resize(obj) ){
    if ( handleIndex == 0 ){
      return tri.P3();
    }
    else if ( handleIndex == 1 ){
      return tri.P2();
    }
    else if ( handleIndex == 2 ){
      return tri.P3();
    }
    else if ( handleIndex == 3 ){
      return tri.P0();
    }
  }
  else{
    Rect rect(bounding_rect(tri));
    if ( handleIndex == 0 ){
      return rect.BottomRight();
    }
    else if ( handleIndex == 1 ){
      return rect.BottomLeft();
    }
    else if ( handleIndex == 2 ){
      return rect.TopRight();
    }
    else if ( handleIndex == 3 ){
      return rect.TopLeft();
    }
  }
  assert(false);
  return Point(0,0);
}

Point origin_from_handle(Object* obj, int handleIndex){
  auto pts = object_aligned_resize(obj) ?
    points(obj->GetTri()) :
    corners(obj->GetRect());
  assert( to_size_t(handleIndex) < pts.size() );
  return pts[handleIndex];
}

RotateObjectTask::RotateObjectTask(Object* obj, int handleIndex, MoveMode mode )
  : m_copy(mode==MoveMode::COPY),
    m_handle(origin_from_handle(obj, handleIndex)),
    m_object(obj),
    m_oldTri(obj->GetTri()),
    m_pivot(pivot_from_handle(obj, handleIndex))
{
  m_object->SetActive();
  if ( m_copy ){
    // Set the pending command for the copied object here, so that
    // it is freed if the command is left unretrieved.
    m_command.Set( new AddObjectCommand(m_object, select_added(true), "Rotate Clone" ) );
  }
}

TaskResult RotateObjectTask::Commit(){
  m_object->ClearActive();
  if ( !m_copy ){
    m_command.Set(new TriCommand(m_object, New(m_object->GetTri()), Old(m_oldTri), "Rotate"));
  }
  return TASK_COMMIT_AND_CHANGE;
}

void RotateObjectTask::Draw( FaintDC& dc, Overlays& overlays, const Point& ){
  if ( m_copy ){
    m_object->Draw(dc);
  }
  overlays.Pivot(m_pivot);
}

bool RotateObjectTask::DrawBeforeZoom(Layer) const{
  return false;
}

Command* RotateObjectTask::GetCommand(){
  return m_command.Retrieve();
}

Cursor RotateObjectTask::GetCursor(const CursorPositionInfo&) const{
  return Cursor::ROTATE;
}

Task* RotateObjectTask::GetNewTask(){
  return nullptr;
}

IntRect RotateObjectTask::GetRefreshRect( const IntRect&, const Point& ) const{
  return inflated(m_object->GetRefreshRect(), faint::objectHandleWidth);
}

TaskResult RotateObjectTask::LeftDown( const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult RotateObjectTask::LeftUp( const CursorPositionInfo& ){
  return Commit();
}

TaskResult RotateObjectTask::Motion( const CursorPositionInfo& info ){
  const Point& pos = info.pos;
  faint::radian a1 = rad_angle( m_pivot.x, m_pivot.y, m_handle.x, m_handle.y );
  faint::radian a2 = rad_angle( m_pivot.x, m_pivot.y, pos.x, pos.y );
  faint::coord delta = a2 - a1;
  if ( fl(TOOLMODIFIER2, info.modifiers ) ){
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

  info.status->SetText( str_degrees( delta * faint::degreePerRadian ) );

  Tri t2( rotated( m_oldTri, delta, m_pivot ) );
  m_object->SetTri( t2 );
  return TASK_DRAW;
}

TaskResult RotateObjectTask::Preempt( const CursorPositionInfo& ){
  return Commit();
}
