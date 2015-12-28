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

#ifndef FAINT_PENBEHAVIOR_HH
#define FAINT_PENBEHAVIOR_HH
#include "toolbehavior.hh"
#include <vector>

class PenBehavior : public ToolBehavior {
public:
  PenBehavior();
  bool DrawBeforeZoom(Layer) const;
  int GetCursor( const CursorPositionInfo& );
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  ToolRefresh Preempt();
  IntRect GetRefreshRect( const IntRect& visible, const Point& currPos );
  bool Draw( FaintDC&, Overlays&, const Point& currPos);
  unsigned int GetStatusFieldCount();
  Command* GetCommand();
private:
  constrain_dir::ConstrainDir m_constrainDir;
  bool m_active;
  std::vector<IntPoint> m_points;
  IntPoint m_origin;
  PendingCommand m_command;
};

#endif
