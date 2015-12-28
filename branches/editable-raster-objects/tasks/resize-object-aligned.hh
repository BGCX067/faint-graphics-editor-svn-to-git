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

#ifndef FAINT_RESIZE_OBJECT_ALIGNED_HH
#define FAINT_RESIZE_OBJECT_ALIGNED_HH
#include "tasks/move-object.hh" // For MoveMode

class ResizeObjectAlignedTask : public Task {
public:
  ResizeObjectAlignedTask( Object*, int handleIndex, MoveMode );
  void Draw( FaintDC&, Overlays&, const Point& ) override;
  bool DrawBeforeZoom(Layer) const override;
  Command* GetCommand() override;
  Task* GetNewTask() override;
  Cursor GetCursor(const CursorPositionInfo& ) const override;
  IntRect GetRefreshRect( const IntRect&, const Point& ) const override;
  TaskResult LeftDown( const CursorPositionInfo& ) override;
  TaskResult LeftUp( const CursorPositionInfo& ) override;
  TaskResult Motion( const CursorPositionInfo& ) override;
  TaskResult Preempt( const CursorPositionInfo& ) override;
private:
  ResizeObjectAlignedTask& operator=( const ResizeObjectAlignedTask& );
  PendingCommand m_command;
  bool m_copy;
  Object* m_object;
  const Tri m_oldTri;
  int m_handleIndex;
};

#endif
