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

#ifndef FAINT_RESIZEOBJECTTASK_HH
#define FAINT_RESIZEOBJECTTASK_HH
#include "tasks/move-object.hh" // Fixme: For MoveMode

class ResizeObjectTask : public Task {
public:
  ResizeObjectTask( Object*, int handleIndex, MoveMode::Type );
  bool Draw( FaintDC&, Overlays&, const Point& );
  bool DrawBeforeZoom(Layer::type) const;
  Command* GetCommand();
  Task* GetNewTask();
  Cursor::type GetCursor(const CursorPositionInfo& ) const;
  IntRect GetRefreshRect( const IntRect&, const Point& ) const;
  TaskResult LeftDown( const CursorPositionInfo& );
  TaskResult LeftUp( const CursorPositionInfo& );
  TaskResult Motion( const CursorPositionInfo& );
  TaskResult Preempt( const CursorPositionInfo& );
private:
  ResizeObjectTask& operator=( const ResizeObjectTask& ); // Prevent assignment
  PendingCommand m_command;
  bool m_copy;
  bool m_lockX;
  bool m_lockY;
  Object* m_object;
  const Tri m_oldTri;
  Point m_p0;
  Point m_origin;
};

#endif
