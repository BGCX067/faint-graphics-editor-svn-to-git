// -*- coding: us-ascii-unix -*-
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
#include "commands/move-point-cmd.hh"
#include "geo/adjust.hh"
#include "geo/angle.hh"
#include "geo/line.hh"
#include "geo/measure.hh"
#include "objects/object.hh"
#include "rendering/overlay.hh"
#include "tasks/object-selection-move-point.hh"
#include "text/formatting.hh"
#include "util/canvas.hh"
#include "util/convenience.hh"
#include "util/grid.hh"
#include "util/object-util.hh"
#include "util/optional.hh"

namespace faint{

static std::vector<Point> get_corners(const Object* object, int pointIndex){
  const int numPoints = object->NumPoints();
  assert(pointIndex < numPoints);
  const bool firstPoint = pointIndex == 0;
  const bool lastPoint = pointIndex == numPoints - 1;
  const bool closedObject = object->CyclicPoints();
  if ((firstPoint || lastPoint) && !closedObject){
    // Can't find corners for the first and last points in a
    // non-cyclic list of points
    return std::vector<Point>();
  }
  Point p0 = firstPoint ? object->GetPoint(numPoints - 1) : object->GetPoint(pointIndex - 1);
  Point p1 = lastPoint ? object->GetPoint(0) : object->GetPoint(pointIndex + 1);

  return { Point(p0.x, p1.y), Point(p1.x, p0.y) };
}

static Angle get_constrain_angle(const Point& p0, const Point& oldPos){
  // Fixme: Consider angle360. Also unary - suspicious for Angle
  Angle angle = -line_angle({p0, oldPos});
  if (angle < Angle::Zero()){
    return 2 * pi + angle;
  }
  return angle;
}

class MovePointTask : public Task {
public:
  MovePointTask(Object* object, int pointIndex, const Point& oldPos)
    : m_object(object),
      m_oldPos(oldPos),
      m_pointIndex(pointIndex)
  {
    m_object->SetActive();
  }

  void Draw(FaintDC&, Overlays& overlays, const Point&) override{
    if (m_constrainPos.IsSet()){
      overlays.ConstrainPos(m_constrainPos.Get());
    }
  }

  bool DrawBeforeZoom(Layer) const override{
    return false;
  }

  Command* GetCommand() override{
    return m_command.Retrieve();
  }

  Cursor GetCursor(const PosInfo&) const override{
    return Cursor::CROSSHAIR;
  }

  Task* GetNewTask() override{
    return nullptr;
  }

  IntRect GetRefreshRect(const IntRect&, const Point&) const override{
    return m_object->GetRefreshRect();
  }

  TaskResult MouseDown(const PosInfo&) override{
    return TASK_NONE;
  }

  TaskResult MouseUp(const PosInfo&) override{
    return Commit();
  }

  TaskResult MouseMove(const PosInfo& info) override{
    Point p(info.pos);
    bool snapHeld = info.modifiers.Primary();
    bool constrainHeld = info.modifiers.Secondary();
    if (!constrainHeld){
      m_constrainPos.Clear();
    }

    if (neither(constrainHeld, snapHeld)){
      info.status.SetMainText("Ctrl=Snap, Shift=Constrain");
    }
    else if (constrainHeld && !snapHeld){
      info.status.SetMainText("Ctrl+Shift=Constrain other");
    }
    else{
      info.status.SetMainText("");
    }

    // Fixme: Extract to method, returning, e.g.
    // p = maybe_snap(...);
    if (constrainHeld && snapHeld){
      // Constrain the point to the next or previous
      Point nextPt(next_point(m_object, m_pointIndex));
      p = adjust_to_default(nextPt, p, pi/4,
        get_constrain_angle(nextPt, m_oldPos));
      m_constrainPos.Set(nextPt);
      }
    else if (constrainHeld) {
      Point prevPt(prev_point(m_object, m_pointIndex));
      p = adjust_to_default(prevPt, p, pi/4, get_constrain_angle(prevPt, m_oldPos));
      m_constrainPos.Set(prevPt);
    }
    else if (snapHeld){
      // Snap to objects and corners formed by the points
      objects_t objects = info.canvas.GetObjects();
      remove(m_object, from(objects));
      p = snap(p, objects, info.canvas.GetGrid(), get_corners(m_object, m_pointIndex));
    }

    m_object->SetPoint(p, m_pointIndex);
    utf8_string  statusString = m_object->StatusString();
    info.status.SetText(statusString.empty() ?
      str(p) : statusString, 0);
    return TASK_DRAW;
  }

  TaskResult Preempt(const PosInfo&) override{
    return Commit();
  }

private:
  TaskResult Commit(){
    m_object->ClearActive();
    m_command.Set(new MovePointCommand(m_object, m_pointIndex, New(m_object->GetPoint(m_pointIndex)), Old(m_oldPos)));
    return TASK_COMMIT_AND_CHANGE;
  }

  PendingCommand m_command;
  Optional<Point> m_constrainPos;
  Object* m_object;
  Point m_oldPos;
  int m_pointIndex;
};

Task* move_point_task(Object* object, int pointIndex, const Point& oldPos){
  return new MovePointTask(object, pointIndex, oldPos);
}

}
