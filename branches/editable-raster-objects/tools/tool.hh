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

#ifndef FAINT_TOOL_HH
#define FAINT_TOOL_HH
#include "commands/command.hh"
#include "gui/cursors.hh"
#include "tools/tool-id.hh"
#include "util/canvasinterface.hh"
#include "util/commonfwd.hh"
#include "util/cursorposinfo.hh"

enum ToolResult{
  // Return values for Tool methods.
  TOOL_NONE, // Action had no externally relevant effect
  TOOL_DRAW, // Tool wants to draw
  TOOL_COMMIT, // Tool has a command ready
  TOOL_CHANGE, // Tool wishes to uhm switch to a new tool somehow
  TOOL_CANCEL // Tool aborted
};

enum ToolModifiers{
  // Modifiers for Tool methods.
  TOOLMODIFIER1=1, // Ctrl held (Fixme: Make the actual key configurable)
  TOOLMODIFIER2=2, // Shift held (Fixme: Make the actual key configurable)
  LEFT_MOUSE=4, // Left mouse click (Fixme: Do not handle as a modifier)
  RIGHT_MOUSE=8}; // Right mouse click (Fixme: Do not handle as a modifier)


enum ToolSettingMode{
  // When a tool is a setting-eater (EAT_SETTINGS), it will control
  // (some) of the shown settings, and will handle tool setting changes internally -
  // preventing the new setting from becoming an application setting.
  //
  // With RELAY_SETTINGS, the tool will be notified of setting
  // changes, but the application settings will also be changed.
  //
  // EAT_SETTINGS is intended for tools which have some kind of
  // stateful selection that are the targets of the changes, so that
  // for example a selected object's line width can be changed without
  // changing the application-wide tool setting.
  RELAY_SETTINGS,
  EAT_SETTINGS
};

bool is_tool_modifier( int key );

class Tool {
public:
  Tool( ToolId, SettingNotifier& s=get_null_notifier(), ToolSettingMode=RELAY_SETTINGS );
  virtual ~Tool();
  virtual bool AcceptsPastedText() const;

  // Whether the tool currently allows redoing commands in the image.
  // Since tools can also feature undo/redo, it would be surprising
  // if the image-redo was triggered while an undo/redoable tool
  // was active (even when the tool doesn't have anything to redo).
  virtual bool AllowsGlobalRedo() const;
  virtual bool CanRedo() const;
  virtual bool CanUndo() const;
  virtual ToolResult Char( const KeyInfo& );
  virtual bool CopyText( faint::utf8_string& out, bool clear ); // Fixme: Use Unique for clear
  virtual ToolResult Delete();
  virtual ToolResult Deselect();
  virtual void Draw( FaintDC&, Overlays&, const Point& mousePos ) = 0;

  // Determines how the tool output is scaled: Some tools are meant to
  // be drawn to the background-image, and scaled along with the image
  // for zoom, others are meant to be smoothly zoomable, and are drawn
  // on top of the scaled background, adjusting for zoom level and
  // offsets as appropriate.
  virtual bool DrawBeforeZoom(Layer) const=0;

  // True if the tool monopolizes tool-setting changes, or if it
  // allows changes to the common tool settings. In practice, probably
  // only used for making object selection setting changes affect the
  // objects, and leave tool settings unchanged.
  bool EatsSettings() const;
  virtual Command* GetCommand();
  virtual Cursor GetCursor( const CursorPositionInfo& ) const = 0;
  ToolId GetId() const;

  // The (in image coordinates)-rectangle the tool has modified
  virtual IntRect GetRefreshRect( const IntRect& visible, const Point& mousePos ) const = 0;
  virtual std::string GetRedoName() const;
  const Settings& GetSettings() const;
  virtual unsigned int GetStatusFieldCount() const;
  virtual std::string GetUndoName() const;

  // True if the tool cares about selection-related actions
  virtual bool HasSelection() const;
  virtual ToolResult LeftDoubleClick( const CursorPositionInfo& );
  virtual ToolResult LeftDown( const CursorPositionInfo& ) = 0;
  virtual ToolResult LeftUp( const CursorPositionInfo& ) = 0;
  virtual ToolResult Motion( const CursorPositionInfo& ) = 0;
  virtual void Paste( const faint::utf8_string& );

  // The tool is given a chance to commit before being replaced
  virtual ToolResult Preempt( const CursorPositionInfo& ) = 0;
  bool PreventsGlobalRedo() const;
  virtual void Redo();

  // Whether the tool requires a refresh when pointer leaves drawing
  // area. Not called during mouse capture, i.e. while mouse held.
  virtual bool RefreshOnMouseOut();
  virtual ToolResult SelectAll();
  virtual void SelectionChange();
  template<typename T> bool Set( const T&, typename T::ValueType );
  virtual ToolResult ToolSettingUpdate( const Settings& ); // Fixme: Change to bool and consider removing virtual
  virtual void Undo();
protected:
  Settings m_settings;
private:
  Tool& operator=( const Tool& ); // Prevent copy
  ToolId m_id;
  SettingNotifier& m_relay;
  bool m_settingEater;
};

template<typename T>
bool Tool::Set( const T& s, typename T::ValueType value ) {
  if ( m_settings.Has( s ) ){
    m_settings.Set( s, value );
    m_relay.Notify( s, value );
    return true;
  }
  return false;
}

#endif
