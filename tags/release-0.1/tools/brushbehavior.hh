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

#ifndef FAINT_BRUSHBEHAVIOR_HH
#define FAINT_BRUSHBEHAVIOR_HH
#include "toolbehavior.hh"
#include <vector>

class BrushBehavior : public ToolBehavior {
public:
  BrushBehavior();
  bool DrawBeforeZoom(Layer) const;
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  bool MouseOut();
  ToolRefresh Preempt();
  unsigned int GetStatusFieldCount();
  IntRect GetRefreshRect( const IntRect& visible );
  IntRect GetRefreshRect( const IntRect& visible, const Point& mousePos );
  int GetCursor( const CursorPositionInfo& );
  bool Draw( FaintDC&, Overlays&, const Point& currPos );
  Command* GetCommand();
  ToolId GetId() const;
private:
  bool m_active;
  std::vector<IntPoint> m_points;
  IntPoint m_origin;
  bool m_constrain;
  bool m_drawCursor;
  PendingCommand m_command;
};

#endif
