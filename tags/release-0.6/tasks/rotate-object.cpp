#include <cassert>
#include "commands/addobjectcommand.hh"
#include "commands/tricommand.hh"
#include "objects/object.hh"
#include "rendering/overlay.hh"
#include "tasks/rotate-object.hh"
#include "util/angle.hh"
#include "util/flag.hh"
#include "util/formatting.hh"

// Returns the position of the opposite corner of the indicated handle
Point pivot_from_handle(const Object* obj, int handleIndex ){
  Rect objRect(bounding_rect(obj->GetTri()));
  if ( handleIndex == 0 ){
    return objRect.BottomRight();
  }
  else if ( handleIndex == 1 ){
    return objRect.BottomLeft();
  }
  else if ( handleIndex == 2 ){
    return objRect.TopRight();
  }
  else if ( handleIndex == 3 ){
    return objRect.TopLeft();
  }
  assert(false);
  return Point(0,0);
}

RotateObjectTask::RotateObjectTask(Object* obj, int handleIndex, MoveMode::Type mode )
  : m_copy(mode==MoveMode::COPY),
    m_handle(corners(obj->GetRect())[handleIndex]),
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

bool RotateObjectTask::Draw( FaintDC& dc, Overlays& overlays, const Point& ){
  if ( m_copy ){
    m_object->Draw(dc);
  }
  overlays.Pivot(m_pivot);
  return true;
}

bool RotateObjectTask::DrawBeforeZoom(Layer::type) const{
  return false;
}

Command* RotateObjectTask::GetCommand(){
  return m_command.Retrieve();
}

Cursor::type RotateObjectTask::GetCursor(const CursorPositionInfo&) const{
  return Cursor::ROTATE_RIGHT;
}

Task* RotateObjectTask::GetNewTask(){
  return 0;
}

IntRect RotateObjectTask::GetRefreshRect( const IntRect&, const Point& ) const{
  return inflated(m_object->GetRefreshRect(), 5); // Fixme: Inflate is for clearing handles. Use a constant.
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
