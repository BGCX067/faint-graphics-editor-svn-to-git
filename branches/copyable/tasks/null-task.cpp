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

#include "tasks/null-task.hh"

namespace faint{

class NullTask : public Task{
public:
  void Draw(FaintDC&, Overlays&, const Point&) override{
  }

  bool DrawBeforeZoom(Layer) const override{
    return false;
  }

  Command* GetCommand(){
    return nullptr;
  }

  Cursor GetCursor(const PosInfo&) const override{
    return Cursor::DONT_CARE;
  }

  Task* GetNewTask() override{
    return nullptr;
  }

  IntRect GetRefreshRect(const IntRect&, const Point&) const override{
    return IntRect();
  }

  TaskResult MouseDown(const PosInfo&) override{
    return TASK_NONE;
  }

  TaskResult MouseUp(const PosInfo&) override{
    return TASK_NONE;
  }

  TaskResult MouseMove(const PosInfo&) override{
    return TASK_NONE;
  }

  TaskResult Preempt(const PosInfo&) override{
    return TASK_NONE;
  }
};

Task* null_task(){
  return new NullTask();
}

}
