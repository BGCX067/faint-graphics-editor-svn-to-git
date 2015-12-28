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

#include "commands/add-object-cmd.hh"
#include "commands/tri-cmd.hh"
#include "geo/grid.hh"
#include "gui/gui-constants.hh"
#include "objects/object.hh"
#include "rendering/overlay.hh"
#include "tasks/resize-object-aligned.hh"
#include "util/flag.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"

static TaskResult maybe_set_resize_command( bool copy, PendingCommand& cmd, Object* object, const Tri& oldTri ){
  object->ClearActive();
  if ( !copy ){
    cmd.Set( new TriCommand(object, New(object->GetTri()), Old(oldTri), "Resize") );
  }
  return TASK_COMMIT_AND_CHANGE;
}

ResizeObjectAlignedTask::ResizeObjectAlignedTask(Object* object, int handleIndex, MoveMode mode )
  : m_copy(mode == MoveMode::COPY),
    m_object(object),
    m_oldTri(object->GetTri()),
    m_handleIndex(handleIndex)
{
  if ( m_copy ){
    // Set the pending command for the copied object here, so that
    // it is freed if the command is left unretrieved.
    m_command.Set( new AddObjectCommand(m_object,  select_added(true), "Resize Clone") );
  }
  m_object->SetActive();
}

void ResizeObjectAlignedTask::Draw( FaintDC& dc, Overlays& overlays, const Point& ){
  if ( m_copy ){
    m_object->Draw(dc);
  }
  if ( m_object->ShowSizeBox() ){
    if ( object_aligned_resize(m_object) ){
      overlays.Parallelogram( m_object->GetTri() );
    }
    else{
      overlays.Rectangle( bounding_rect(m_object->GetTri()) );
    }
  }
}

bool ResizeObjectAlignedTask::DrawBeforeZoom(Layer) const {
  return false;
}

Command* ResizeObjectAlignedTask::GetCommand(){
  return m_command.Retrieve();
}

Task* ResizeObjectAlignedTask::GetNewTask(){
  return nullptr;
}

Cursor ResizeObjectAlignedTask::GetCursor(const CursorPositionInfo&) const{
  return Cursor::ARROW;
}

IntRect ResizeObjectAlignedTask::GetRefreshRect( const IntRect&, const Point& ) const{
  return inflated(m_object->GetRefreshRect(), faint::objectHandleWidth);
}

TaskResult ResizeObjectAlignedTask::LeftDown( const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult ResizeObjectAlignedTask::LeftUp( const CursorPositionInfo& ){
  return maybe_set_resize_command( m_copy, m_command, m_object, m_oldTri);
}

TaskResult ResizeObjectAlignedTask::Motion( const CursorPositionInfo& info ){
  Point p = info.pos;
  if ( fl(TOOLMODIFIER1, info.modifiers) ){
    objects_t objects = info.canvas->GetObjects();
    if ( !m_copy ){
      // Don't snap to points in this object
      remove(m_object, from(objects));
    }
    p = snap(p, objects, info.canvas->GetGrid());
  }
  else if ( fl(TOOLMODIFIER2, info.modifiers) ){
    if ( m_handleIndex == 0 || m_handleIndex == 3 ){
      p = projection(p, unbounded(LineSegment(m_oldTri.P0(), m_oldTri.P3())));
    }
    else if ( m_handleIndex == 1 || m_handleIndex == 2 ){
      p = projection(p, unbounded(LineSegment(m_oldTri.P1(), m_oldTri.P2())));
    }
  }

  // Note: Incorrect when skewed
  if ( m_handleIndex == 0 ){
    Point p3 = m_oldTri.P3();
    Line p1p3 = unbounded(LineSegment(m_oldTri.P1(), p3));
    Line p2p3 = unbounded(LineSegment(m_oldTri.P2(), p3));
    Point p1 = projection(p, p1p3);
    Point p2 = projection(p, p2p3);
    m_object->SetTri( Tri(p, p1, p2) );
  }
  else if ( m_handleIndex == 1 ){
    Point p2 = m_oldTri.P2();
    Line p0p2 = unbounded(LineSegment(m_oldTri.P0(), p2));
    Point p0 = projection(p, p0p2);
    m_object->SetTri( Tri(p0, p, p2) );
  }
  else if ( m_handleIndex == 2 ){
    Point p1 = m_oldTri.P1();
    Line p0p1 = unbounded(LineSegment(m_oldTri.P0(), p1));
    Line p1p3 = unbounded(LineSegment(p1, m_oldTri.P3()));
    Point p0 = projection(p, p0p1);
    m_object->SetTri(Tri(p0, p1, p));
  }
  else if ( m_handleIndex == 3 ){
    Point p0 = m_oldTri.P0();
    Line lp1 = unbounded(LineSegment(p0, m_oldTri.P1()));
    Line lp2 = unbounded(LineSegment(p0, m_oldTri.P2()));

    Point p1 = projection(p, lp1);
    Point p2 = projection(p, lp2);

    m_object->SetTri( Tri(p0, p1, p2) );
  }
  else if ( m_handleIndex == 4 ){
    // Left side
    Line upper = unbounded(LineSegment(m_oldTri.P0(), m_oldTri.P1()));
    Line lower = unbounded(LineSegment(m_oldTri.P2(), m_oldTri.P3()));
    Point p0 = projection(p, upper);
    Point p2 = projection(p, lower);
    m_object->SetTri( Tri(p0, m_oldTri.P1(), p2 ) );
  }
  else if ( m_handleIndex == 5 ){
    // Right side
    Line upper = unbounded(LineSegment(m_oldTri.P0(), m_oldTri.P1()));
    Line lower = unbounded(LineSegment(m_oldTri.P2(), m_oldTri.P3()));
    Point p1 = projection(p, upper);
    m_object->SetTri( Tri(m_oldTri.P0(), p1, m_oldTri.P2() ) );
  }
  else if ( m_handleIndex == 6 ){
    // Upper
    Line left = unbounded(LineSegment(m_oldTri.P2(), m_oldTri.P0()));
    Line right = unbounded(LineSegment(m_oldTri.P3(), m_oldTri.P1()));
    Point p0 = projection(p, left);
    Point p1 = projection(p, right);
    m_object->SetTri(Tri(p0, p1, m_oldTri.P2()));
  }
  else if ( m_handleIndex == 7 ){
    // Lower
    Line left = unbounded(LineSegment(m_oldTri.P2(), m_oldTri.P0()));
    Line right = unbounded(LineSegment(m_oldTri.P3(), m_oldTri.P1()));
    Point p2 = projection(p, left);
    m_object->SetTri(Tri(m_oldTri.P0(), m_oldTri.P1(), p2));
  }

  return TASK_DRAW;
}

TaskResult ResizeObjectAlignedTask::Preempt( const CursorPositionInfo& ){
  return maybe_set_resize_command(m_copy, m_command, m_object, m_oldTri);
}
