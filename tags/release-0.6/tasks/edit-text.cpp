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

#include <cassert>
#include "app/getappcontext.hh"
#include "commands/textentrycommand.hh"
#include "objects/objtext.hh"
#include "rendering/overlay.hh"
#include "tasks/edit-text.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"
#include "util/util.hh"

inline bool is_exit_key( key::key_t keycode ){
  return keycode == key::ctrl_enter || keycode == key::esc;
}

inline bool outside(Object* obj, const CursorPositionInfo& info){
  return !obj->GetRect().Contains(info.pos);
}

inline bool right_click( const CursorPositionInfo& info ){
  return fl(RIGHT_MOUSE, info.modifiers);
}

inline bool select(key::mod_t mod){
  return key::shift_held(mod);
}

EditText::EditText( const Rect& r, const faint::utf8_string& str, Settings& settings )
  : m_active(false),
    m_newTextObject(true),
    m_settings(settings),
    m_textObject( new ObjText(r, str, settings) )
{}

EditText::EditText( ObjText* obj, Settings& s )
  : m_active(false),
  m_newTextObject(false),
  m_oldText(obj->GetTextBuffer()->get()),
  m_settings( s ),
  m_textObject( obj )
{
  m_settings = m_textObject->GetSettings();
}

EditText::~EditText(){
  if ( m_active ){
    EndEntry();
  }
}

void EditText::Activate(){
  assert(!m_active);
  m_textObject->SetActive(true);
  m_textObject->SetEdited(true);
  TextBuffer* buf(m_textObject->GetTextBuffer());
  buf->caret(buf->size() - 1);
  GetAppContext().BeginTextEntry();
  m_active = true;
}

bool EditText::AcceptsPastedText() const{
  return true;
}

TaskResult EditText::Char( const KeyInfo& info ){
  if ( is_exit_key( info.keyCode ) ){
    m_command.Set(CreateCommand(info.layerType));
    return TASK_COMMIT_AND_CHANGE;
  }

  TextBuffer* text = m_textObject->GetTextBuffer();
  if ( HandleSpecialKey( info.keyCode, info.keyModifiers, *text ) ){
    return TASK_DRAW;
  }
  text->insert(info.ch);
  return TASK_DRAW;
}

bool EditText::CopyText( faint::utf8_string& str, bool clear ){
  str = m_textObject->GetTextBuffer()->get_selection();
  if ( clear && !str.empty() ){
    m_textObject->GetTextBuffer()->del();
  }
  return true;
}

Command* EditText::CreateCommand(Layer::type layerType){
  if ( m_active ){
    EndEntry();
  }
  if ( m_newTextObject ){
    return add_or_draw(m_textObject, layerType);
  }
  // Fixme: Only if changed
  return new TextEntryCommand( m_textObject, New(m_textObject->GetTextBuffer()->get()), Old(m_oldText) ); // Fixme: Change type in TextBuffer, and add accessor to ObjText
}

TaskResult EditText::Delete(){
  m_textObject->GetTextBuffer()->del();
  return TASK_DRAW;
}

TaskResult EditText::Deselect(){
  m_textObject->GetTextBuffer()->select_none();
  return TASK_DRAW;
}

bool EditText::Draw( FaintDC& dc, Overlays& overlays, const Point& ){
  overlays.Corners( m_textObject->GetTri() );
  if ( m_newTextObject ){
    m_textObject->Draw( dc );
  }
  overlays.Caret( m_textObject->GetCaret() );
  return true;
}

bool EditText::DrawBeforeZoom(Layer::type layer) const{
  return layer == Layer::RASTER; // Fixme: What happens on layer switch with existing object?
}

void EditText::EndEntry(){
  assert( m_active );

  GetAppContext().EndTextEntry();
  if ( m_textObject != 0 ){
    m_textObject->SetActive(false);
    m_textObject->SetEdited(false);
  }
  m_active = false;
}

Command* EditText::GetCommand(){
  return m_command.Retrieve();
}

Cursor::type EditText::GetCursor( const CursorPositionInfo& info ) const{
  if ( m_textObject->GetTri().Contains(info.pos) ){
    return Cursor::CARET;
  }
  return Cursor::ARROW; // Fixme: Allow switching objects?
}

Task* EditText::GetNewTask(){
  return DefaultTask();
}

IntRect EditText::GetRefreshRect( const IntRect&, const Point& ) const{
  return inflated( m_textObject->GetRefreshRect(), 5, 5 ); // Fixme: Inflate needed?
}

bool EditText::HandleSpecialKey( key::key_t keycode, key::mod_t mod, TextBuffer& text ){
  if ( keycode == key::back ){
    text.del_back();
    return true;
  }
  else if ( keycode == key::del ){
    return true;
  }
  else if ( keycode == key::enter ){
    text.insert(faint::eol);
    return true;
  }
  else if ( keycode == key::left ){
    text.devance( select(mod) );
    return true;
  }
  else if ( keycode == key::right ){
    text.advance( select(mod) );
    return true;
  }
  else if ( keycode == key::up ){
    text.move_up(select(mod));
    return true;
  }
  else if ( keycode == key::down ){
    text.move_down(select(mod));
    return true;
  }
  else if ( keycode == key::home ){
    size_t newPos = text.prev( faint::eol );
    text.caret( newPos == 0 ? newPos : newPos + 1, select(mod) );
    return true;
  }
  else if ( keycode == key::end ){
    text.caret( text.next(faint::eol), select(mod) );
    return true;
  }
  else if ( keycode == key::ctrl_b ){
    m_textObject->Set( ts_FontBold, m_textObject->GetSettings().Not( ts_FontBold ) );
    return true;
  }
  else if ( keycode == key::ctrl_i && key::ctrl_held(mod) ) { // Ctrl+I has the same keycode as TAB, so check that CTRL is held // Fixme: Use a key-class or whatever
    m_textObject->Set( ts_FontItalic, m_textObject->GetSettings().Not( ts_FontItalic ) );
    return TASK_DRAW;
  }
  return false;
}

bool EditText::HasSelection() const{
  return true;
}

TaskResult EditText::LeftDown( const CursorPositionInfo& info ){
  if ( m_textObject == 0 ){
    // Fixme: How?
    return TASK_NONE;
  }
  if ( right_click(info) || outside(m_textObject, info ) ){
    m_command.Set(CreateCommand(info.layerType));
    return TASK_COMMIT_AND_CHANGE;
  }
  return TASK_NONE;
}

TaskResult EditText::LeftUp( const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult EditText::Motion( const CursorPositionInfo& info ){
  info.status->SetMainText("Use Ctrl+Enter to stop editing");
  info.status->SetText(str(info.pos));
  return TASK_NONE;
}

void EditText::Paste( const faint::utf8_string& str ){
  m_textObject->GetTextBuffer()->insert(str);
}

TaskResult EditText::Preempt( const CursorPositionInfo& info ){
  m_command.Set(CreateCommand(info.layerType));
  return TASK_COMMIT_AND_CHANGE;
}

TaskResult EditText::SelectAll(){
  TextBuffer* text = m_textObject->GetTextBuffer();
  text->caret(0);
  text->caret(text->size(), true);
  return TASK_DRAW;
}

bool EditText::UpdateSettings(){
  return m_textObject->UpdateSettings(m_settings);
}
