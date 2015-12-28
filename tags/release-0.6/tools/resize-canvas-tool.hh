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
#include "tools/tool.hh"

class ResizeCanvas : public Tool {
public:
  enum Operation { Resize, Rescale };
  enum Direction{ UP_DOWN, LEFT_RIGHT, DIAGONAL };
  ResizeCanvas( const IntPoint& handlePoint, const IntPoint& oppositePoint, const IntSize& imageSize, Operation, Direction );
  bool Draw( FaintDC&, Overlays&, const Point& );
  bool DrawBeforeZoom(Layer::type) const;
  Command* GetCommand();
  Cursor::type GetCursor( const CursorPositionInfo& ) const;
  IntRect GetRefreshRect( const IntRect&, const Point& ) const;
  ToolResult LeftDown( const CursorPositionInfo& );
  ToolResult LeftUp( const CursorPositionInfo& );
  ToolResult Motion( const CursorPositionInfo& );
  ToolResult Preempt( const CursorPositionInfo& );
private:
  Command* CreateCommand( const CursorPositionInfo& );
  const IntPoint m_origin;
  const IntPoint m_opposite;
  PendingCommand m_command;
  IntPoint m_release;
  IntSize m_imageSize;
  Operation m_operation;
  Direction m_dir;
  ScaleQuality::type m_quality;
};

#endif
