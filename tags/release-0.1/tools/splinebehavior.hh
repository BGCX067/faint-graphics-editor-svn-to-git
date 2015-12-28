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

#ifndef FAINT_SPLINEBEHAVIOR_HH
#define FAINT_SPLINEBEHAVIOR_HH
#include "toolbehavior.hh"
#include "geo/points.hh"

class SplineCommand : public Command{
public:
  SplineCommand( const Points&, const FaintSettings& );
  virtual void Do( faint::Image& img );
private:
  Points m_points;
  FaintSettings m_settings;
};


class SplineBehavior : public ToolBehavior{
public:
  SplineBehavior();
  bool DrawBeforeZoom(Layer) const;

  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  ToolRefresh Preempt();
  bool Draw( FaintDC&, Overlays&, const Point& );
  IntRect GetRefreshRect( const IntRect&, const Point& currPos );
  virtual Command* GetCommand();
  virtual int GetCursor(const CursorPositionInfo& );
  unsigned int GetStatusFieldCount();
private:
  Point m_p1;
  Point m_p2;
  bool m_active;
  Points m_points;
  int m_otherButton;
  PendingCommand m_command;
};

#endif
