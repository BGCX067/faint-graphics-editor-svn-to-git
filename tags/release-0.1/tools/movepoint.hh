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

#ifndef FAINT_MOVEPOINT_HH
#define FAINT_MOVEPOINT_HH
#include "toolbehavior.hh"

class MovePointBehavior : public ToolBehavior {
public:
  MovePointBehavior( Object*, size_t pointIndex, ToolBehavior* prevTool, bool copy );
  bool DrawBeforeZoom(Layer) const;
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  ToolRefresh Preempt();
  bool Draw( FaintDC&, Overlays&, const Point& );
  IntRect GetRefreshRect( const IntRect& visible, const Point& );
  Command* GetCommand();
  int GetCursor( const CursorPositionInfo& );
  ToolBehavior* GetNewTool();
private:
  bool m_active;
  Object* m_object;
  size_t m_pointIndex;
  Point m_oldPos;
  faint::radian m_oldAngle;
  ToolBehavior* m_prevTool;
  bool m_copy;
};

#endif
