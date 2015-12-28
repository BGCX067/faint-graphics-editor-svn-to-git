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

#ifndef FAINT_TEXTBEHAVIOR_HH
#define FAINT_TEXTBEHAVIOR_HH
#include "toolbehavior.hh"
#include "commands/command.hh"
#include "objects/objtext.hh"
class ObjText;

class TextCommand : public Command{
public:
  TextCommand( ObjText* );
  ~TextCommand();
  void Do( faint::Image& );
private:
  ObjText* m_textObj;
};

class TextBehavior : public ToolBehavior{
public:
  TextBehavior();
  ~TextBehavior();
  TextBehavior( ObjText*, ToolBehavior* prevTool = 0 );
  bool DrawBeforeZoom(Layer) const;
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&,  int modifiers );
  ToolRefresh Preempt();
  ToolRefresh Char( wchar_t, int keycode, int modifiers );
  bool Draw( FaintDC&, Overlays&, const Point& currPos );
  IntRect GetRefreshRect( const IntRect& , const Point& );
  Command* GetCommand();
  int GetCursor( const CursorPositionInfo& );
  bool CopyData( std::string&, bool clear );
  ToolRefresh Delete();
  void Paste( const std::string& );
  bool AcceptsPastedText();
  unsigned int GetStatusFieldCount();
  ToolBehavior* GetNewTool();

  template <typename T>
  void NotifySetting( const T& s, typename T::ValueType value ){
    if ( m_textObject != 0 ){
      m_textObject->Set( s, value );
    }
  }
private:
  enum TextState{ INACTIVE, ACTIVE, DRAWING_RECTANGLE };
  void BeginRectangle();
  void BeginTextEntry( bool );
  ToolRefresh EndTextEntry();
  void PrepareCommit();
  TextState m_state;
  bool m_owner;
  ObjText* m_textObject;
  // Bounding box corners - relevant only while drawing rectangle.
  Point m_rect_1;
  Point m_rect_2;
  TargettedNotifier< TextBehavior > m_notifier;
  ToolBehavior* m_prevTool;
  wxString m_oldText;
  PendingCommand m_command;
};

#endif
