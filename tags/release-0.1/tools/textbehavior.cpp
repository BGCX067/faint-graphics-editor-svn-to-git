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

#include "textbehavior.hh"
#include "commands/addobjectcommand.hh"
#include "cursors.hh"
#include "faintdc.hh"
#include "getappcontext.hh"
#include "objects/objrectangle.hh"
#include "objects/objtext.hh"
#include "settingid.hh"
#include "util/util.hh"

using std::min;
using std::max;

bool select( int mod ){
  return wxMOD_SHIFT == (mod & wxMOD_SHIFT);
}

TextCommand::TextCommand( ObjText* text )
  : Command( CMD_TYPE_RASTER ),
    m_textObj( text )
{}

void TextCommand::Do( faint::Image& img ){
  FaintDC dc( img.GetBitmapRef() );
  m_textObj->Draw( dc );
}

TextCommand::~TextCommand(){
  delete m_textObj;
}

TextBehavior::TextBehavior()
  : ToolBehavior( T_TEXT, m_notifier )
{
  m_notifier.SetTarget( this );
  m_state = INACTIVE;
  m_prevTool = 0;
  m_textObject = 0;
  m_settings = GetTextSettings();
}

TextBehavior::TextBehavior( ObjText* textObj, ToolBehavior* prevTool )
  : ToolBehavior( T_TEXT ),
    m_textObject( textObj ),
    m_prevTool( prevTool )
{
  m_settings = GetTextSettings();
  BeginTextEntry( false );
  TextBuffer& text = *( m_textObject->GetTextBuffer() );
  text.caret( text.size() );
}

bool TextBehavior::DrawBeforeZoom( Layer layer ) const{
  return layer == LAYER_RASTER;
}

void TextBehavior::BeginTextEntry( bool owner ){
  m_owner = owner;
  m_state = ACTIVE;
  m_oldText = m_textObject->GetTextBuffer()->get();
  m_textObject->SetEdited( true );
  m_textObject->SetActive();

  Rect r = m_textObject->GetRect();
  if ( r.w < 20 && r.h < 20 ){
    // Tiny rect - Make the text object resize on its own
    m_textObject->Set( ts_TextAutoRect, true );
  }
  else {
    // Enforce a minimum size if the rect is too low or narrow
    r.w = max( r.w, 20.0 );
    r.h = max( r.h, 20.0 );
    m_textObject->Set( ts_TextAutoRect, false );
    // m_textObject->SetArea( r.x, r.y, r.x + r.w, r.y + r.h );
  }
  GetAppContext().BeginTextEntry();
}

ToolRefresh TextBehavior::EndTextEntry(){
  assert( m_textObject != 0 );

  m_state = INACTIVE;
  wxString newText = m_textObject->GetTextBuffer()->get();
  GetAppContext().EndTextEntry();
  if ( m_owner ){
    PrepareCommit();
    return TOOL_COMMIT;
  }

  // Editing existing object
  m_command.Set( new TextEntryCommand( m_textObject, newText, m_oldText ) );
  m_textObject->ClearActive();
  m_textObject->SetEdited( false );
  m_textObject = 0;
  if ( m_prevTool != 0 ){
    return TOOL_CHANGE;
  }
  return TOOL_COMMIT;
}

TextBehavior::~TextBehavior(){
  if ( m_state == ACTIVE ){
    EndTextEntry();
  }
  if ( m_owner ){
    delete m_textObject;
  }
  else if ( m_textObject != 0 ) {
    m_textObject->SetEdited( false );
  }
}

ToolRefresh TextBehavior::Delete(){
  TextBuffer* text = m_textObject->GetTextBuffer();
  text->del();
  return TOOL_OVERLAY;
}

void TextBehavior::PrepareCommit(){
  assert( m_owner );
  m_state = INACTIVE;
  m_textObject->SetEdited( false );
  m_textObject->ClearActive();
  if ( GetAppContext().GetLayerType() == LAYER_RASTER ){
    m_command.Set( new TextCommand( m_textObject ) );
  }
  else {
    m_command.Set( new AddObjectCommand( m_textObject ) );
  }
  m_textObject = 0;
}

ToolRefresh TextBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  if ( modifiers & RIGHT_MOUSE ){
    // Right click ends text entry
    return m_state == ACTIVE ?
      EndTextEntry() :
      TOOL_NONE;
  }

  if ( info.layerType == LAYER_OBJECT && info.objSelected ){
    // Clicked inside a selected object, if it's a text object, start
    // editing that object instead, and position the caret
    ObjText* tObj = dynamic_cast<ObjText*>( info.object );
    if ( tObj ){
      if ( tObj == m_textObject ){
        // Fixme: Clicked inside current object position the caret
        return TOOL_NONE;
      }
      else {
        // Clicked a different text object - switch to that object
        // Fixme: Add caret positioning
        ToolRefresh result = TOOL_OVERLAY;
        if ( m_state == ACTIVE ){
          result = EndTextEntry();
        }
        m_textObject = tObj;
        BeginTextEntry( false );
        return result;
      }
    }
  }

  if ( m_state == ACTIVE ){
    if ( Rect( m_rect_1, m_rect_2 ).Contains( info.pos ) ) {
      // Fixme: clicked in tool-area - add caret positioning here!
      return TOOL_NONE;
    }
    else{
      // Clicked outside while in typing mode, end text entry
      return EndTextEntry();
    }
  }
  else {
    // Begin drawing a region for text-entry
    m_state = DRAWING_RECTANGLE;
    m_rect_1 = info.pos;
    m_rect_2 = info.pos;
    return TOOL_OVERLAY;
  }
}

ToolRefresh TextBehavior::LeftUp( const CursorPositionInfo&, int modifiers){
  if ( m_state == DRAWING_RECTANGLE && modifiers & LEFT_MOUSE ) {
    Rect r(m_rect_1, m_rect_2);
    if ( Empty(r) ){
      m_state = INACTIVE;
      return TOOL_NONE;
    }
    FaintSettings settings( GetTextSettings() );
    settings.Update( GetAppContext().GetToolSettings() );
    m_textObject = new ObjText( r, "", settings );
    m_textObject->SetActive();
    BeginTextEntry( true );
    return TOOL_OVERLAY;
  }
  return TOOL_NONE;
}

ToolRefresh TextBehavior::Motion( const CursorPositionInfo& info, int ){
  StatusInterface& status = GetAppContext().GetStatusInfo();
  if ( m_state == DRAWING_RECTANGLE ){
    m_rect_1 = info.pos;
    status.SetText( StrFromTo( m_rect_2, m_rect_1 ), 0 );
    status.SetMainText("");
    return TOOL_OVERLAY;
  }
  else if ( m_state == ACTIVE ) {
    status.SetMainText("Use Ctrl+Enter or right mouse to stop text entry.");
    status.SetText("",0);
  }
  else {
    status.SetText( StrPoint( info.pos ), 0 );
    status.SetMainText("Hold left mouse to define a text rectangle" );
  }

  return TOOL_NONE;
}

ToolRefresh TextBehavior::Char( wchar_t ch, int keycode, int mod) {
  if ( m_state != ACTIVE ){
    return TOOL_NONE;
  }

  TextBuffer* text = m_textObject->GetTextBuffer();
  const int CTRL_A = 1;
  const int CTRL_B = 2;
  const int CTRL_D = 4;
  const int CTRL_I = 9;
  const int CTRL_ENTER = 10;  

  if ( keycode == WXK_BACK ){
    text->del_back();
  }
  else if ( keycode == WXK_NUMPAD_DELETE ||
    keycode == WXK_DELETE ||
    keycode == CTRL_D ){
    text->del();
  }
  else if ( keycode == WXK_RETURN ){
    text->insert(TextBuffer::eol);
  }
  else if ( keycode == WXK_LEFT ){
    text->devance( select(mod) );
  }
  else if ( keycode == WXK_RIGHT ){
    text->advance( select(mod) );
  }
  else if ( keycode == WXK_UP ){
    text->move_up(select(mod));
  }
  else if ( keycode == WXK_DOWN ){
    text->move_down(select(mod));
  }
  else if ( keycode == WXK_HOME ){
    size_t newPos = text->prev( TextBuffer::eol );
    text->caret( newPos == 0 ? newPos : newPos + 1, select(mod) );
  }
  else if ( keycode == WXK_END ){
    text->caret( text->next(TextBuffer::eol), select(mod) );
  }
  else if ( keycode == CTRL_A ){
    // Select all
    text->caret(0);
    text->caret( text->size(), true );
  }
  else if ( keycode == CTRL_B ){
    m_textObject->Set( ts_FontBold, m_textObject->GetSettings().Not( ts_FontBold ) );
  }

  else if ( keycode == CTRL_I &&
    wxMOD_CONTROL == (mod & wxMOD_CONTROL) ){ // Ctrl+I has the same keycode as TAB, so check that CTRL is held
    m_textObject->Set( ts_FontItalic, m_textObject->GetSettings().Not( ts_FontItalic ) );
  }
  else if ( keycode == CTRL_ENTER || keycode == WXK_ESCAPE ){
    return EndTextEntry();
  }
  else {
    text->insert( ch );
  }
  return TOOL_OVERLAY;
}

bool TextBehavior::Draw( FaintDC& dc, Overlays& overlays, const Point& ){
  if ( m_state ==  INACTIVE){
    return true;
  }

  if ( m_state == ACTIVE ){
    overlays.Rectangle( m_textObject->GetRect() );
    if ( m_owner ){
      m_textObject->Draw( dc );
    }

    overlays.Caret( m_textObject->GetCaret() );
  }

  else if ( m_state == DRAWING_RECTANGLE ){
    overlays.Rectangle( Rect( m_rect_1, m_rect_2 ) );
  }
  return true;
}

int TextBehavior::GetCursor( const CursorPositionInfo& info ){
  if ( info.layerType == LAYER_RASTER && m_state == ACTIVE
    && m_textObject->GetRect().Contains( info.pos ) ){
    return CURSOR_CARET;
  }

  else if ( info.layerType == LAYER_OBJECT && info.objSelected ){
    ObjText* tObj = dynamic_cast<ObjText*>( info.object );
    if ( tObj ){
      return CURSOR_CARET;
    }
  }

  return CURSOR_TEXT_CROSSHAIR;
}

ToolRefresh TextBehavior::Preempt(){
  if ( m_state == ACTIVE ){
    return EndTextEntry();
  }
  return TOOL_NONE;
}

IntRect TextBehavior::GetRefreshRect( const IntRect&, const Point& ){
  if ( m_state == DRAWING_RECTANGLE ){   
    return Inflated( IntRect(truncated(m_rect_1), truncated(m_rect_2)), 5 );
  }
  else if ( m_textObject == 0 ){
    return IntRect(IntPoint(0,0),IntSize(0,0));
  }
  else {
    return Inflated( m_textObject->GetRefreshRect(), 5, 5 );
  }
}

Command* TextBehavior::GetCommand(){
  return m_command.Retrieve();
}

ToolBehavior* TextBehavior::GetNewTool(){
  assert( m_prevTool != 0 );
  return m_prevTool;
}

bool TextBehavior::CopyData( std::string& str, bool clear ){
  if ( m_state != ACTIVE ){
    return false;
  }
  str = m_textObject->GetTextBuffer()->get_selection();

  if ( clear && !str.empty() ){
    m_textObject->GetTextBuffer()->del();
  }
  return true;
}

bool TextBehavior::AcceptsPastedText(){
  if ( m_state == ACTIVE ){
    return true;
  }
  else {
    return false;
  }
}

void TextBehavior::Paste( const std::string& str ){
  m_textObject->GetTextBuffer()->insert( str );
}

unsigned int TextBehavior::GetStatusFieldCount(){
  return 1;
}
