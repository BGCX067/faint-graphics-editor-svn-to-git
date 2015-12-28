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

#ifndef FAINT_POLYGONBEHAVIOR_HH
#define FAINT_POLYGONBEHAVIOR_HH
#include "toolbehavior.hh"
#include "geo/points.hh"
#include "settings.hh"

class PolygonCommand : public Command {
public:
  PolygonCommand( const Points&, const FaintSettings& );
  void Do( faint::Image& );

private:
  Points m_points;
  FaintSettings m_settings;
};

class PolygonBehavior : public ToolBehavior {
public:
  PolygonBehavior();  
  bool DrawBeforeZoom(Layer) const;
  ToolRefresh LeftDoubleClick( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  ToolRefresh Preempt();
  bool Draw( FaintDC&, Overlays&, const Point& );
  int GetCursor( const CursorPositionInfo& );
  IntRect GetRefreshRect( const IntRect&, const Point& );
  Command* GetCommand();
  unsigned int GetStatusFieldCount();
private:
  void Reset();
  Point ConstrainPoint( const Point&, int modifiers );
  ToolRefresh AddPoint( const Point&, int mouseButton );
  ToolRefresh Commit( int layerType, const Points&, const FaintSettings& );
  Points m_points;
  bool m_active;
  int m_mouseButton;
  PendingCommand m_command;
};

#endif
