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

#ifndef FAINT_RESIZECANVAS_HH
#define FAINT_RESIZECANVAS_HH
#include "toolbehavior.hh"
#include "commands/rescalecommand.hh"

class ResizeCanvas : public ToolBehavior {
public:
  enum Operation { Resize, Rescale };
  enum Direction{ UP_DOWN, LEFT_RIGHT, DIAGONAL };
  ResizeCanvas( ToolBehavior* prevTool, const IntPoint& handlePoint, const IntPoint& oppositePoint, const IntSize& imageSize, Operation, Direction );
  bool DrawBeforeZoom(Layer) const;
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  ToolRefresh Preempt();
  bool Draw( FaintDC& dc, Overlays&, const Point& currPos );
  IntRect GetRefreshRect( const IntRect& visible, const Point& currPos );
  Command* GetCommand();
  int GetCursor( const CursorPositionInfo& );
  ToolBehavior* GetNewTool();
private:
  const IntSize m_imageSize;
  const IntPoint m_opposite;
  IntPoint m_release;
  ToolBehavior* m_prevTool;
  Operation m_operation;
  Direction m_direction;
  RescaleCommand::Quality m_quality;
};

#endif
