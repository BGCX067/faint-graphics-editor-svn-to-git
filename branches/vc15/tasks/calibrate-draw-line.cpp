// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#include "app/resource-id.hh"
#include "tasks/standard-task.hh"
#include "tasks/calibrate-tasks.hh"
#include "geo/geo-func.hh"
#include "geo/int-rect.hh"
#include "geo/measure.hh"
#include "geo/line.hh"
#include "geo/rect.hh"
#include "rendering/overlay.hh"
#include "util/pos-info.hh"

namespace faint{

class CalibrateDrawLine : public StandardTask{
  // Task for drawing a line for calibrating the image coordinate
  // system.
public:
  CalibrateDrawLine(const PosInfo& info)
    : m_line(info.pos, info.pos)
  {
    info.status.SetMainText("Draw measuring line.");
  }

  virtual void Draw(FaintDC&, Overlays& overlays, const PosInfo&) override{
    overlays.Line(m_line);
  }

  virtual bool DrawBeforeZoom(Layer) const override{
    return false;
  }

  Command* GetCommand() override{
    return nullptr;
  }

  Cursor GetCursor(const PosInfo&) const override{
    return Cursor::CALIBRATE_CROSSHAIR;
  }

  Task* GetNewTask() override{
    return m_newTask.Take();
  }

  IntRect GetRefreshRect(const RefreshInfo&) const override{
    return floiled(bounding_rect(m_line));
  };

  TaskResult MouseDown(const PosInfo&) override{
    // The CalibrateDrawLine task is created on MouseDown, and
    // terminates on MouseUp.
    return TaskResult::NONE;
  }

  TaskResult MouseUp(const PosInfo& info) override{
    m_line.p1 = info.pos;

    if (length(m_line) != 0){
      m_newTask.Set(calibrate_enter_measure(m_line, info));
    }
    return TaskResult::CHANGE;
  }

  TaskResult MouseMove(const PosInfo& info) override{
    m_line.p1 = info.pos;
    return TaskResult::DRAW;
  }

  TaskResult Preempt(const PosInfo&) override{
    return TaskResult::CHANGE;
  }

private:
  LineSegment m_line;
  PendingTask m_newTask;
};

Task* calibrate_draw_line(const PosInfo& info){
  return new CalibrateDrawLine(info);
}

} // namespace
