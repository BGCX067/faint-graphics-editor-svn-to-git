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

#ifndef FAINT_MOVEOBJECTTASK_HH
#define FAINT_MOVEOBJECTTASK_HH
#include "geo/tri.hh"
#include "tasks/task.hh"

enum class MoveMode{MOVE, COPY};

class MoveObjectTask : public Task {
public:
  MoveObjectTask( Object* mainObject,
    const objects_t& allObjects,
    const Point& offset,
    MoveMode mode=MoveMode::MOVE );
  void Draw( FaintDC&, Overlays&, const Point& ) override;
  bool DrawBeforeZoom(Layer) const override;
  Command* GetCommand() override;
  Cursor GetCursor( const CursorPositionInfo& ) const override;
  Task* GetNewTask() override;
  IntRect GetRefreshRect( const IntRect&, const Point& ) const override;
  TaskResult LeftDown( const CursorPositionInfo& ) override;
  TaskResult LeftUp( const CursorPositionInfo& ) override;
  TaskResult Motion( const CursorPositionInfo& ) override;
  TaskResult Preempt( const CursorPositionInfo& ) override;
private:
  TaskResult Commit();
  Point SnapObject( objects_t, const Grid& );
  PendingCommand m_command;
  bool m_copy;
  bool m_moved;
  Object* m_mainObject;
  objects_t m_objects;
  Point m_offset;
  Tri m_oldTri;
  std::vector<Tri> m_origTris;
  Rect m_refreshRect;
};

#endif
