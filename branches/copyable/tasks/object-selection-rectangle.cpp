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

#include "geo/geo-func.hh"
#include "objects/object.hh"
#include "rendering/overlay.hh"
#include "tasks/object-selection-rectangle.hh"
#include "util/active-canvas.hh"
#include "util/object-util.hh"

namespace faint{

static bool add_enclosed(const PosInfo& info){
  // Whether the surrounded objects should be added to the current set
  // of objects, or set as the selection, losing any previous
  // selection
  return info.modifiers.Secondary();
}

static bool remove_enclosed(const PosInfo& info){
  // Whether the enclosed objects should be removed from the current
  // selection
  return info.modifiers.Primary();
}

static void modify_selection(const PosInfo& info, const objects_t& enclosed){
  if (enclosed.empty()){
    return;
  }

  // Perform the modification depending on modifiers
  if (remove_enclosed(info)){
    info.canvas.DeselectObjects(enclosed);
  }
  else {
    info.canvas.SelectObjects(enclosed, deselect_old(!add_enclosed(info)));
  }
}

class SelectObjectRect : public Task {
public:
  SelectObjectRect(const Point& p0, const Point& p1, Settings& settings, const ActiveCanvas& canvas)
    : m_canvas(canvas),
      m_p0(p0),
      m_p1(p1),
      m_settings(settings)
  {}

  void Draw(FaintDC&, Overlays& overlays, const Point& p) override{
    m_p1 = p;
    overlays.Rectangle(Rect(m_p0, m_p1));
  }

  bool DrawBeforeZoom(Layer) const override{
    return false;
  }

  Cursor GetCursor(const PosInfo&) const override{
    return Cursor::CROSSHAIR;
  }

  Command* GetCommand() override{
    return nullptr;
  }

  Task* GetNewTask() override{
    return nullptr;
  }

  IntRect GetRefreshRect(const IntRect&, const Point&) const override{
    return floored(inflated(Rect(m_p0, m_p1), 2));
  }

  TaskResult MouseDown(const PosInfo&) override{
    // Shan't happen
    return TASK_NONE;
  }

  TaskResult MouseUp(const PosInfo& info) override{
    const objects_t enclosed = get_intersected(info.canvas.GetObjects(), Rect(m_p0, m_p1));
    modify_selection(info, enclosed);
    return TASK_CHANGE;
  }

  TaskResult MouseMove(const PosInfo&) override{
    return TASK_DRAW;
  }

  TaskResult Preempt(const PosInfo&) override{
    return TASK_CHANGE;
  }

  void SelectionChange() override{
    m_settings = get_object_settings(m_canvas->GetObjectSelection());
  }

  SelectObjectRect& operator=(const SelectObjectRect&) = delete;
private:
  ActiveCanvas m_canvas;
  Point m_p0;
  Point m_p1;
  Settings& m_settings;
};

Task* select_object_rectangle_task(const Point& p0, const Point& p1, Settings& s, const ActiveCanvas& canvas)
{
  return new SelectObjectRect(p0, p1, s, canvas);
}

} // namespace
