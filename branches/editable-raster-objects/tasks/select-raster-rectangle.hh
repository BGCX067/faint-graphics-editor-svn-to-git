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
#ifndef FAINT_SELECTRASTERRECTANGLE_HH
#define FAINT_SELECTRASTERRECTANGLE_HH
#include "tasks/raster-selection-task.hh"

class SelectRectangle : public RasterSelectionTask {
public:
  SelectRectangle( const Point& startPos, Settings&, bool m_appendCommand );
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
  SelectRectangle& operator=(const SelectRectangle&); // Prevent assignment
  PendingCommand m_command;
  Point m_p0;
  Point m_p1;
  faint::coord m_maxDistance;
  bool m_appendCommand;
};

#endif
