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

#ifndef FAINT_MOVEPOINTTASK_HH
#define FAINT_MOVEPOINTTASK_HH
#include "tasks/task.hh"
#include "util/unique.hh"

struct MovePointMode{
  // Modifies the MovePointTask
  enum Type{
    MOVE, // Move the indicated point
    INSERT, // Insert a point at the indicated index and move the new point
    CLONE // Clone the object and move the indicated point in the new object
  };
};

class MovePointTask : public Task {
public:
  MovePointTask( Object*, size_t pointIndex, MovePointMode::Type );
  bool Draw( FaintDC&, Overlays&, const Point& );
  bool DrawBeforeZoom(Layer::type) const;
  Command* GetCommand();
  Cursor::type GetCursor(const CursorPositionInfo&) const;
  Task* GetNewTask();
  IntRect GetRefreshRect( const IntRect&, const Point& ) const;
  TaskResult LeftDown( const CursorPositionInfo& );
  TaskResult LeftUp( const CursorPositionInfo& );
  TaskResult Motion( const CursorPositionInfo& );
  TaskResult Preempt( const CursorPositionInfo& );
private:
  TaskResult Commit();
  PendingCommand m_command;
  MovePointMode::Type m_mode;
  Object* m_object;
  Object* m_originalObject; // Necessary?
  size_t m_pointIndex; // Necessary?
  Point m_oldPos; // Necessary?
  Optional<Point> m_constrainPos;
  faint::radian m_nextAngle;
  faint::radian m_prevAngle;
};

#endif
