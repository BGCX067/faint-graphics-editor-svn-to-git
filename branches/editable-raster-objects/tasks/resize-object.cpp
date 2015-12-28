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
#include "tasks/resize-object.hh"
#include "util/flag.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"

// Fixme: The class ResizeObjectTask mostly duplicates the newer class
// ResizeObjectAligned.

static TaskResult maybe_set_resize_command( bool copy, PendingCommand& cmd, Object* object, const Tri& oldTri ){
  object->ClearActive();
  if ( !copy ){
    cmd.Set( new TriCommand(object, New(object->GetTri()), Old(oldTri), "Resize") );
  }
  return TASK_COMMIT_AND_CHANGE;
}

ResizeObjectTask::ResizeObjectTask(Object* object, int handleIndex, MoveMode mode )
  : m_copy(mode == MoveMode::COPY),
    m_lockX(false),
    m_lockY(false),
    m_object(object),
    m_oldTri(object->GetTri()),
    m_p0(0,0),
    m_origin(0,0)
{

  if ( m_copy ){
    // Set the pending command for the copied object here, so that
    // it is freed if the command is left unretrieved.
    m_command.Set( new AddObjectCommand(m_object,  select_added(true), "Resize Clone") );
  }
  m_object->SetActive();
  Rect objRect(bounding_rect(m_oldTri));
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
    m_lockY = true;
  }
  else if ( handleIndex == 5 ){
    m_p0 = Point( objRect.Right(), objRect.Center().y );
    m_origin = Point( objRect.Left(), objRect.Center().y );
    m_lockY = true;
  }
  else if ( handleIndex == 6 ){
    m_p0 = Point( objRect.Center().x , objRect.y );
    m_origin = Point( objRect.Center().x, objRect.Bottom() );
    m_lockX = true;
  }
  else if ( handleIndex == 7 ){
    m_p0 = Point( objRect.Center().x, objRect.Bottom() );
    m_origin = Point( objRect.Center().x , objRect.y );
    m_lockX = true;
  }
}

void ResizeObjectTask::Draw( FaintDC& dc, Overlays& overlays, const Point& ){
  if ( m_copy ){
    m_object->Draw(dc);
  }
  if ( m_object->ShowSizeBox() ){
    overlays.Rectangle( bounding_rect(m_object->GetTri()) );
  }
}

bool ResizeObjectTask::DrawBeforeZoom(Layer) const {
  return false;
}

Command* ResizeObjectTask::GetCommand(){
  return m_command.Retrieve();
}

Task* ResizeObjectTask::GetNewTask(){
  return nullptr;
}

Cursor ResizeObjectTask::GetCursor(const CursorPositionInfo&) const{
  return Cursor::ARROW;
}

IntRect ResizeObjectTask::GetRefreshRect( const IntRect&, const Point& ) const{
  return inflated(m_object->GetRefreshRect(), faint::objectHandleWidth);
}

TaskResult ResizeObjectTask::LeftDown( const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult ResizeObjectTask::LeftUp( const CursorPositionInfo& ){
  return maybe_set_resize_command( m_copy, m_command, m_object, m_oldTri);
}

TaskResult ResizeObjectTask::Motion( const CursorPositionInfo& info ){
  Point p1 = info.pos;
  if ( fl(TOOLMODIFIER1, info.modifiers) ){
    objects_t objects = info.canvas->GetObjects();
    if ( !m_copy ){
      // Don't snap to yourself
      remove(m_object, from(objects));
    }
    if ( m_lockY ){
      Rect oldRect( bounding_rect( m_oldTri ) );
      p1.x = snap_x( p1.x, objects, info.canvas->GetGrid(), oldRect.Top(), oldRect.Bottom() );
    }
    else if ( m_lockX ){
      Rect oldRect( bounding_rect( m_oldTri ) );
      p1.y = snap_y( p1.y, objects, info.canvas->GetGrid(), oldRect.Left(), oldRect.Right() );
    }
    else {
      p1 = snap( p1, objects, info.canvas->GetGrid() );
    }
  }


  double dx = m_p0.x - m_origin.x;
  faint::coord scale_x = (m_lockX || dx == 0 )? 1.0 :
    ( p1.x - m_origin.x ) / dx;

  double dy = m_p0.y - m_origin.y;
  faint::coord scale_y = ( m_lockY || dy == 0 ) ? 1.0 :
    ( p1.y - m_origin.y ) / dy;

  bool constrain = fl( TOOLMODIFIER2, info.modifiers );
  if ( constrain && !m_lockX && !m_lockY ) {
    scale_y = ( (scale_x < 0) == (scale_y < 0) ) ? scale_x : - scale_x;
  }

  m_object->SetTri( scaled( m_oldTri, Scale(scale_x, scale_y), m_origin ) );
  info.status->SetText( str_scale( scale_x, scale_y ) );
  return TASK_DRAW;
}

TaskResult ResizeObjectTask::Preempt( const CursorPositionInfo& ){
  return maybe_set_resize_command(m_copy, m_command, m_object, m_oldTri);
}
