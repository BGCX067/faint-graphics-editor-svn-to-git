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

#ifndef FAINT_POLYGON_TOOL_HH
#define FAINT_POLYGON_TOOL_HH
#include "geo/points.hh"
#include "tools/tool.hh"
#include "util/undo.hh"

class PolygonTool : public Tool {
public:
  PolygonTool();
  bool AllowsGlobalRedo() const;
  bool CanRedo() const;
  bool CanUndo() const;
  bool Draw( FaintDC&, Overlays&, const Point& );
  bool DrawBeforeZoom(Layer::type) const;
  Command* GetCommand();
  Cursor::type GetCursor( const CursorPositionInfo& ) const;
  std::string GetRedoName() const;
  IntRect GetRefreshRect( const IntRect&, const Point& ) const;
  std::string GetUndoName() const;
  ToolResult LeftDoubleClick( const CursorPositionInfo& );
  ToolResult LeftDown( const CursorPositionInfo& );
  ToolResult LeftUp( const CursorPositionInfo& );
  ToolResult Motion( const CursorPositionInfo& );
  ToolResult Preempt( const CursorPositionInfo& );
  void Redo();
  void Undo();
private:
  void Reset();
  Point ConstrainPoint( const Point&, int modifiers );
  ToolResult AddPoint( const Point&, int mouseButton );
  ToolResult Commit( Layer::type );
  Points m_points;
  UndoRedo<Point> m_states;
  bool m_active;
  int m_mouseButton;
  PendingCommand m_command;
};

#endif
