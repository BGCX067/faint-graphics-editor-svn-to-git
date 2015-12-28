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

#ifndef FAINT_PEN_TOOL_HH
#define FAINT_PEN_TOOL_HH
#include <vector>
#include "tools/tool.hh"
#include "util/util.hh" // constrain_dir

class PenTool : public Tool {
public:
  PenTool();
  void Draw( FaintDC&, Overlays&, const Point& ) override;
  bool DrawBeforeZoom(Layer) const override;
  Command* GetCommand() override;
  Cursor GetCursor( const CursorPositionInfo& ) const override;
  IntRect GetRefreshRect( const IntRect&, const Point& ) const override;
  ToolResult LeftDown( const CursorPositionInfo& ) override;
  ToolResult LeftUp( const CursorPositionInfo& ) override;
  ToolResult Motion( const CursorPositionInfo& ) override;
  ToolResult Preempt( const CursorPositionInfo& ) override;
private:
  ToolResult Commit();
  constrain_dir::ConstrainDir m_constrainDir;
  bool m_active;
  std::vector<IntPoint> m_points;
  IntPoint m_origin;
  PendingCommand m_command;
};

#endif
