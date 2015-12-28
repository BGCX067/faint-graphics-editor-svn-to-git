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
#include "commands/add-point-cmd.hh"
#include "commands/point-cmd.hh"
#include "geo/grid.hh"
#include "objects/object.hh"
#include "rendering/overlay.hh"
#include "tasks/move-point.hh"
#include "util/angle.hh"
#include "util/flag.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"
#include "util/util.hh"

std::vector<Point> get_corners( const Object* object, size_t pointIndex ){
  size_t numPoints = object->NumPoints();
  assert( pointIndex < numPoints );
  const bool firstPoint = pointIndex == 0;
  const bool lastPoint = pointIndex == numPoints - 1;
  const bool closedObject = object->CyclicPoints();
  if ( ( firstPoint || lastPoint ) && !closedObject){
    // Can't find corners for the first and last points in a
    // non-cyclic list of points
    return std::vector<Point>();
  }
  Point p0 = firstPoint ? object->GetPoint(numPoints - 1) : object->GetPoint(pointIndex - 1);
  Point p1 = lastPoint ? object->GetPoint(0) : object->GetPoint( pointIndex + 1 );
  std::vector<Point> pts;
  pts.push_back(Point(p0.x, p1.y));
  pts.push_back(Point(p1.x, p0.y));
  return pts;
}


MovePointTask::MovePointTask(Object* object, size_t pointIndex, MovePointMode::Type mode )
  : m_mode(mode),
    m_object(object),
    m_originalObject(object),
    m_pointIndex(pointIndex),
    m_oldPos(object->GetPoint(pointIndex))
{
  // Fixme: Leaks m_object when cloning point
  if ( m_mode == MovePointMode::CLONE ){
    // ?
  }
  else if ( m_mode == MovePointMode::INSERT ){
    m_pointIndex += 1;
    m_object = m_originalObject->Clone();
    m_originalObject->SetVisible(false);
    m_object->InsertPoint(m_oldPos, m_pointIndex);
  }
  m_object->SetActive();

  Point nextPt(next_point(m_object, m_pointIndex) );
  Point prevPt(prev_point(m_object, m_pointIndex) );
  m_prevAngle = -rad_angle(prevPt.x, prevPt.y, m_oldPos.x, m_oldPos.y);
  m_nextAngle = -rad_angle(nextPt.x, nextPt.y, m_oldPos.x, m_oldPos.y);
  if ( m_prevAngle < 0 ){
    m_prevAngle = 2 * faint::pi + m_prevAngle;
  }
  if ( m_nextAngle < 0 ){
    m_nextAngle = 2 * faint::pi + m_nextAngle;
  }
}

TaskResult MovePointTask::Commit(){
  m_object->ClearActive();
  m_originalObject->SetVisible(true);

  if ( m_mode == MovePointMode::CLONE ){
    m_command.Set( new AddObjectCommand(m_object, select_added(true), "Endpoint Clone") );
  }
  else if ( m_mode == MovePointMode::INSERT ){
    m_command.Set( new AddPointCommand(m_originalObject, m_pointIndex, m_object->GetPoint(m_pointIndex)) );
  }
  else if ( m_mode == MovePointMode::MOVE ){
    m_command.Set( new PointCommand(m_object, m_pointIndex, New(m_object->GetPoint( m_pointIndex )), Old(m_oldPos)) );
  }
  else {
    assert(false);
  }
  return TASK_COMMIT_AND_CHANGE;
}

bool MovePointTask::Draw( FaintDC& dc, Overlays& overlays, const Point& ){
  if ( m_mode == MovePointMode::CLONE || m_mode == MovePointMode::INSERT ){
    m_object->Draw(dc);
  }

  if ( m_constrainPos.IsSet() ){
    overlays.ConstrainPos(m_constrainPos.Get());
  }
  return true;
}

bool MovePointTask::DrawBeforeZoom(Layer::type) const{
  return false;
}

Command* MovePointTask::GetCommand(){
  return m_command.Retrieve();
}

Cursor::type MovePointTask::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::CROSSHAIR;
}

Task* MovePointTask::GetNewTask(){
  return 0;
}

IntRect MovePointTask::GetRefreshRect( const IntRect&, const Point& ) const{
  return m_object->GetRefreshRect();
}

TaskResult MovePointTask::LeftDown( const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult MovePointTask::LeftUp( const CursorPositionInfo& ){
  return Commit();
}

TaskResult MovePointTask::Motion( const CursorPositionInfo& info ){
  Point p(info.pos);
  bool constrain = fl(TOOLMODIFIER2, info.modifiers);
  if ( !constrain ){
    m_constrainPos.Clear();
  }

  if ( constrain ){
    // Constrain the point to the next or previous
    if ( fl(TOOLMODIFIER1, info.modifiers ) ){
      Point nextPt(next_point(m_object, m_pointIndex));
      p = adjust_to_default( next_point(m_object, m_pointIndex), p, faint::pi/4, m_nextAngle );
      m_constrainPos.Set(nextPt);
    }
    else {
      Point prevPt(prev_point(m_object, m_pointIndex));
      p = adjust_to_default( prevPt, p, faint::pi/4, m_prevAngle );
      m_constrainPos.Set(prevPt);
    }
  }
  else if ( fl(TOOLMODIFIER1, info.modifiers) ){
    // Snap to objects and corners formed by the points
    objects_t objects = info.canvas->GetObjects();
    remove(m_originalObject, from(objects));
    p = snap( p, objects, info.canvas->GetGrid(), get_corners(m_object, m_pointIndex) );
  }

  m_object->SetPoint( p, m_pointIndex );
  std::string status = m_object->StatusString();
  if ( status.empty() ){
    info.status->SetText(str(p),0);
  }
  else {
    info.status->SetText( status, 0 );
  }
  return TASK_DRAW;
}

TaskResult MovePointTask::Preempt( const CursorPositionInfo& ){
  return Commit();
}
