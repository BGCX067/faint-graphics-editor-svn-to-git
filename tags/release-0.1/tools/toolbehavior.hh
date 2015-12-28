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

#ifndef FAINT_TOOLBEHAVIOR_HH
#define FAINT_TOOLBEHAVIOR_HH
#include "bitmap/bitmap.h"
#include "canvasinterface.hh"
#include "commands/command.hh"
#include "cursors.hh"
#include "geo/geotypes.hh"
#include "objects/object.hh"
#include "overlay.hh"
#include "toolid.hh"
#include "util/formatting.hh"
#include "util/util.hh"

enum ToolRefresh{
  TOOL_NONE, // Action had no externally relevant effect
  TOOL_OVERLAY, // Tool wants to draw an overlay
  TOOL_COMMIT, // Tool has a command ready
  TOOL_CHANGE, // Tool wishes to uhm switch to a new tool somehow
  TOOL_CANCEL // Tool aborted
};

enum ToolModifiers{TOOLMODIFIER1=1, TOOLMODIFIER2=2, LEFT_MOUSE=4, RIGHT_MOUSE=8};

enum  TypeModifiers{
  TYPE_MIN=1,
  TYPE_CLEAR_OVERLAY=TYPE_MIN,
  TYPE_DEFAULT=TYPE_CLEAR_OVERLAY,
  TYPE_NO_DC};


struct CursorPositionInfo{
  CanvasInterface* canvas;
  // The position the mouse event occured
  Point pos;
  // True if position is inside a selection region
  bool inSelection;
  // The current layer-type choice
  int layerType;
  // Hitstatus of objects, relevant only if object layer
  int hitStatus;
  // True if object of hitStatus is a selected object.
  // Relevant only if object layer and hitStatus != HIT_NONE
  bool objSelected;
  Object* object;
  int handleIndex; // Relevant only if ...
};

inline int mbtn( int flags ){
  if ( fl( LEFT_MOUSE, flags ) ){
    return LEFT_MOUSE;
  }
  else {
    return RIGHT_MOUSE;
  }
}

class StrBtn {
public:
  StrBtn( int mouse_flag ){
    if ( mouse_flag == LEFT_MOUSE ){
      btnThis = "left";
      btnThat = "right";
    }
    else if ( mouse_flag == RIGHT_MOUSE ){
      btnThis = "right";
      btnThat = "left";
    }
  }
  const std::string This( bool capital ) const {
    if ( capital ){
      // Fixme
      // return btnThis.Capitalize();
    }
    return btnThis;
  }
  const std::string Other( bool capital ) const {
    if ( capital ){
      // return btnThat.Capitalize();
    }
    return btnThat;
  }
private:
  std::string btnThis;
  std::string btnThat;
};

extern const FaintSettings NullSettings;

class NullNotifier : public SettingNotifier {
public:
  virtual void Notify( const BoolSetting&, bool ){};
  virtual void Notify( const IntSetting&, int ){};
  virtual void Notify( const StrSetting&, const std::string& ){}
  virtual void Notify( const ColorSetting&, const faint::Color& ){}
  virtual void Notify( const FloatSetting&, FloatSetting::ValueType ){}
};

extern NullNotifier nullNotifier;

class ToolBehavior {
public:
  ToolBehavior( ToolId, SettingNotifier& s=nullNotifier, bool eatSettings=false );
  virtual ~ToolBehavior();
  virtual ToolRefresh LeftDoubleClick( const CursorPositionInfo&, int modifiers );
  virtual ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers ) = 0;
  virtual ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers ) = 0;
  virtual ToolRefresh Motion( const CursorPositionInfo&,  int modifiers ) = 0;
  virtual bool MouseOut();

  // Deterrmines how the tool output is scaled: Some tools are meant
  // to be drawn to the background-image, and scaled along with the
  // image for zoom, others are meant to be smoothly zoomable,
  // and are drawn on top of the scaled background, adjusting for zoom
  // level and offsets as appropriate.
  virtual bool DrawBeforeZoom(Layer) const=0;
  virtual ToolRefresh Char( wchar_t, int keycode, int modifiers );
  virtual ToolRefresh ToolSettingUpdate( const FaintSettings& );
  virtual bool HasSelection() const;
  virtual ToolRefresh Deselect();
  virtual void SelectionChange();
  virtual bool Draw( FaintDC&, Overlays&, const Point& currPos )=0;

  // The tool is given a chance to commit due to an external event
  // e.g. tool change
  virtual ToolRefresh Preempt()=0;
  virtual bool AcceptsPastedText();

  virtual bool CopyData( std::string& out, bool clear );
  virtual bool CopyData( faint::Bitmap& out );

  virtual ToolRefresh Delete();

  virtual void Paste( const std::string& );

  virtual ToolBehavior* GetNewTool();

  // Fixme: Remove (or move modifiers into cursorposinfo
  virtual int GetCursor( const CursorPositionInfo& );
  virtual int GetCursor( const CursorPositionInfo&, int modifiers );

  // The (in image coordinates)-rectangle the tool has modified
  virtual IntRect GetRefreshRect( const IntRect& visible, const Point& mousePos ) = 0;
  virtual Command* GetCommand();

  // Needn't be virtual, replace with constructor argument
  virtual unsigned int GetStatusFieldCount();

  virtual const FaintSettings& GetSettings();
  bool EatsSettings();

  virtual bool HasBitmap() const;

  virtual faint::Bitmap* GetBitmap();
  virtual void UpdateBitmap();
  ToolId GetId() const;

  template<typename T>
  ToolRefresh Set( const T& s, typename T::ValueType value ){
    if ( m_settings.Has( s ) ){
      m_settings.Set( s, value );
      m_relay.Notify( s, value );
      return TOOL_OVERLAY;
    }

    return TOOL_NONE;
  }

protected:
  FaintSettings m_settings;

private:
  ToolBehavior& operator=( const ToolBehavior& );
  ToolId m_id;
  bool m_settingEater;
  SettingNotifier& m_relay;
};

#endif
