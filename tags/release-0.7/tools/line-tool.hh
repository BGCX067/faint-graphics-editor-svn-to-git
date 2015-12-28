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

#ifndef FAINT_LINE_TOOL_HH
#define FAINT_LINE_TOOL_HH
#include <vector>
#include "tools/tool.hh"
#include "util/undo.hh"

class LineTool : public Tool{
public:
  LineTool();
  bool AllowsGlobalRedo() const;
  bool CanRedo() const;
  bool CanUndo() const;
  ToolResult Char( const KeyInfo& );
  bool Draw( FaintDC&, Overlays&, const Point& );
  bool DrawBeforeZoom(Layer::type) const;
  Command* GetCommand();
  Cursor::type GetCursor( const CursorPositionInfo& ) const;
  std::string GetRedoName() const;
  IntRect GetRefreshRect( const IntRect&, const Point& ) const;
  std::string GetUndoName() const;
  ToolResult LeftDown( const CursorPositionInfo& );
  ToolResult LeftUp( const CursorPositionInfo& );
  ToolResult Motion( const CursorPositionInfo& );
  ToolResult Preempt( const CursorPositionInfo& );
  void Redo();
  void Undo();
private:
  ToolResult UpdatePoints( const CursorPositionInfo&, bool, bool );
  Command* CreateCommand( Layer::type );
  std::vector<Point> m_points;
  bool m_active;
  int m_otherButton;
  PendingCommand m_command;
  UndoRedo<Point> m_states;
};

#endif
