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

#ifndef FAINT_RECTANGLEBEHAVIOR_HH
#define FAINT_RECTANGLEBEHAVIOR_HH
#include "toolbehavior.hh"
#include "objects/objrectangle.hh"
#include "settings.hh"

class RectangleCommand : public Command {
 public:
  RectangleCommand( const Point& p0, const Point& p1, const FaintSettings& s );
  virtual void Do( faint::Image& );

 private:  
  FaintSettings m_settings;
  Point m_p0;
  Point m_p1;
};

class RectangleBehavior : public ToolBehavior {
public:
  RectangleBehavior();
  bool DrawBeforeZoom(Layer) const;
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  ToolRefresh Preempt();
      
  bool Draw( FaintDC&, Overlays&, const Point& currPos );
  int GetCursor(const CursorPositionInfo&);
  IntRect GetRefreshRect( const IntRect&, const Point& currPos );
  unsigned int GetStatusFieldCount();
  Command* GetCommand();

private:  
  bool m_active;
  Point m_p0;
  Point m_p1;
  PendingCommand m_command;
};

#endif
