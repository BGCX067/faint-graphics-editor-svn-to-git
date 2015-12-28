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

#ifndef FAINT_FILLBEHAVIOR_HH
#define FAINT_FILLBEHAVIOR_HH
#include "toolbehavior.hh"
#include "settings.hh"

class FillCommand : public Command{
public:
  FillCommand( const Point& pos, const faint::Color& );
  void Do( faint::Image& );

private:
  Point m_point;
  faint::Color m_fillColor;
};

class FillBehavior : public ToolBehavior {
public:
  FillBehavior();
  bool DrawBeforeZoom(Layer) const;
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  ToolRefresh Preempt();
  bool Draw( FaintDC&, Overlays&, const Point& );
  int GetCursor( const CursorPositionInfo& );
  IntRect GetRefreshRect( const IntRect&, const Point& currPos );
  Command* GetCommand();
  unsigned int GetStatusFieldCount();
private:
  PendingCommand m_command;
};

#endif
