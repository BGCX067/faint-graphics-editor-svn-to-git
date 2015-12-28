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

#ifndef FAINT_ROTATEOBJECT_HH
#define FAINT_ROTATEOBJECT_HH
#include "toolbehavior.hh"

class ObjRotateBehavior : public ToolBehavior {
public:
  ObjRotateBehavior( Object* obj, int handleIndex, ToolBehavior* previous, bool copy );  
  bool DrawBeforeZoom(Layer) const;
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  bool Draw( FaintDC&, Overlays&, const Point& currPos );
  IntRect GetRefreshRect( const IntRect& visible, const Point& currPos);
  Command* GetCommand();
  int GetCursor( const CursorPositionInfo& );
  ToolBehavior* GetNewTool();
  ToolRefresh Preempt();
private:
  Object* m_object;
  Tri m_oldTri;
  Point m_handle;
  Point m_pivot;
  ToolBehavior* m_previousTool;
  bool m_active;
  bool m_copy;
};

#endif
