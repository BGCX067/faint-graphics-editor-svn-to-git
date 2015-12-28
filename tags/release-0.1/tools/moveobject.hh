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

#ifndef FAINT_MOVEOBJECT_HH
#define FAINT_MOVEOBJECT_HH
#include "toolbehavior.hh"

class ObjMoveBehavior : public ToolBehavior {
public:
  ObjMoveBehavior( CanvasInterface*, Object* mainObject, std::vector<Object*> allObjects, ToolBehavior* prevTool, const Point& offset, bool copy=false);
  bool DrawBeforeZoom(Layer) const;
  Object* GetMainObject();
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  ToolRefresh Preempt();
  bool Draw( FaintDC&, Overlays&, const Point& currPos );
  IntRect GetRefreshRect( const IntRect&, const Point& );
  Command* GetCommand();
  int GetCursor( const CursorPositionInfo& );
  ToolBehavior* GetNewTool();
private:
  Point SnapObject( std::vector<Object*>, const Grid& );
  bool m_active;
  bool m_copy;
  Point m_offset;
  ToolBehavior* m_prevTool;
  Object* m_object;

  // The original position before move start
  Tri m_oldTri;

  Point m_pos;
  Rect m_boundingBox;
  Rect m_refreshRect;

  std::vector<Object*> m_objects;
  std::vector<Tri> m_origTris;
};

#endif
