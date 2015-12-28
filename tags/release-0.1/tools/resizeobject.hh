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

#ifndef FAINT_RESIZEOBJECT_HH
#define FAINT_RESIZEOBJECT_HH
#include "toolbehavior.hh"
class Object;

class ObjResizeBehavior : public ToolBehavior {
public:
  ObjResizeBehavior(Object*, int HandleIndex, ToolBehavior* prevTool, bool copy );
  bool DrawBeforeZoom(Layer) const;
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  ToolRefresh Preempt();
  bool Draw( FaintDC&, Overlays&, const Point& currPos );
  
  IntRect GetRefreshRect( const IntRect& visible, const Point& );
  Command* GetCommand();
  int GetCursor( const CursorPositionInfo& );
  ToolBehavior* GetNewTool();
private:
  Object* m_object;
  bool m_lock_x;
  bool m_lock_y;
  const Tri m_oldTri;
  Point m_p0;
  Point m_origin;  
  ToolBehavior* m_prevTool;
  bool m_active;
  bool m_copy;
};

#endif
