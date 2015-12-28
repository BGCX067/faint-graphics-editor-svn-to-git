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

#ifndef FAINT_OBJECT_SELECTION_IDLE_HH
#define FAINT_OBJECT_SELECTION_IDLE_HH
#include "tasks/task.hh"

namespace faint{
class ActiveCanvas;
class SelectObjectIdleImpl;

class SelectObjectIdle : public Task {
public:
  SelectObjectIdle(Settings&, const ActiveCanvas&);
  ~SelectObjectIdle();
  void Activate() override;
  void Draw(FaintDC&, Overlays&, const Point&) override;
  bool DrawBeforeZoom(Layer) const override;
  Command* GetCommand() override;
  Cursor GetCursor(const PosInfo&) const override;
  Task* GetNewTask() override;
  IntRect GetRefreshRect(const IntRect&, const Point&) const override;
  TaskResult DoubleClick(const PosInfo&) override;
  TaskResult MouseDown(const PosInfo&) override;
  TaskResult MouseUp(const PosInfo&) override;
  TaskResult MouseMove(const PosInfo&) override;
  TaskResult Preempt(const PosInfo&) override;
  void SelectionChange() override;

  SelectObjectIdle(const SelectObjectIdle&) = delete;
  SelectObjectIdle& operator=(const SelectObjectIdle&) = delete;
private:
  SelectObjectIdleImpl* m_impl;
  PendingCommand m_command;
};

} // namespace

#endif
