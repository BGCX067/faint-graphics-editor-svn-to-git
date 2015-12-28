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

#ifndef FAINT_OBJSELECTBEHAVIOR_HH
#define FAINT_OBJSELECTBEHAVIOR_HH
#include "commands/command.hh"
#include "toolbehavior.hh"
#include "objects/object.hh"
#include "appcontext.hh"

class ObjSelectBehavior;

class ObjSettingNotifier : public SettingNotifier {
public:
  ObjSettingNotifier();
  void SetTarget( AppContext* );
  virtual void Notify( const BoolSetting&, bool );
  virtual void Notify( const IntSetting&, int );
  virtual void Notify( const StrSetting&, const std::string& );
  virtual void Notify( const ColorSetting&, const faint::Color& );
  virtual void Notify( const FloatSetting&, FloatSetting::ValueType );
private:
  void RunCommand( Command* );
  AppContext* m_context;
};

class ObjSelectBehavior : public ToolBehavior {
public:
  ObjSelectBehavior( AppContext& );
  bool DrawBeforeZoom(Layer) const;
  ToolRefresh LeftDoubleClick( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  ToolRefresh ToolSettingUpdate( const FaintSettings& );
  ToolRefresh Preempt();
  bool Draw( FaintDC&, Overlays&, const Point& currPos );
  const FaintSettings& GetSettings();
  IntRect GetRefreshRect( const IntRect& visible, const Point& );
  ToolBehavior* GetNewTool();
  int GetCursor( const CursorPositionInfo&, int modifiers );
  void SelectionChange();
  unsigned int GetStatusFieldCount();

private:
  void SettingsFromObjects( const std::vector<Object*>& );
  ToolRefresh ClickedNothing( const CursorPositionInfo&, int );
  ToolRefresh ClickedSelected( const CursorPositionInfo&, int );
  ToolRefresh ClickedUnselected( const CursorPositionInfo&, int );
  ToolRefresh CompleteRectangle( const CursorPositionInfo&, int );

  // Switched to on tool-change
  ToolBehavior* m_otherTool;

  // Stores the clicked position to determine
  // if a ctrl-click is a drag or a deselect
  Point m_clickPos;
  bool m_mouseDown;
  Object* m_deselectCandidate;
  Object* m_dragCandidate;

  bool m_drawRectangle;
  Point m_p1;
  Point m_p2;

  // Set if changes affect a hard-to-determine region
  // e.g. all selected objects
  bool m_fullRefresh;
  ObjSettingNotifier m_notifier;
  AppContext& m_appContext;
};

#endif
