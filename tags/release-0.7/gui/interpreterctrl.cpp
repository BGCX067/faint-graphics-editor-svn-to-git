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

#include "interpreterctrl.hh"
#include "util/autocomplete.hh"
#include "util/fileautocomplete.hh"
#include "util/bindkey.hh"
#include "wx/dir.h"
#include "wx/wx.h" // Fixme
#include "python/pyinterface.hh" // Fixme

inline void backward_char( wxTextCtrl& text ){
  text.SetInsertionPoint( text.GetInsertionPoint() -1 );
}

inline void forward_char( wxTextCtrl& text ){
  text.SetInsertionPoint( text.GetInsertionPoint() + 1 );
}

bool in_quote( const wxString& text ){
  int quotes = 0;
  for ( size_t i = 0; i != text.size(); i++ ){
    if ( text[i] == wxChar('\"') && ( i == 0 || text[i-1] != wxChar('\\') ) ){
      quotes++;
    }
  }
  return quotes % 2 != 0;
}

void kill_line( wxTextCtrl& text ){
  long x, y;
  text.PositionToXY( text.GetInsertionPoint(), &x, &y );
  long len = text.GetLineLength( y );
  text.Remove( text.GetInsertionPoint(), text.GetInsertionPoint() + len - x );
}

class AutoCompleteState{
public:
  AutoCompleteState(){
    m_index = 0;
    m_ac.add( "Canvas" );
    m_ac.add( "objects" );
    m_ac.add( "selected" );
    m_ac.add( "images" );
    m_ac.add( "get_selection()" );
    m_ac.add( "set_selection(" );
    m_ac.add( "get_settings()" );
    m_ac.add( "get_active_image()");
    m_ac.add( "list_fonts()" );
    m_ac.add( "print" );
    m_ac.add( "flatten()" );
    m_ac.add( "Rect(" );
    m_ac.add( "Ellipse(" );
    m_ac.add( "Spline(" );
    m_ac.add( "Polygon(" );
    m_ac.add( "Settings()" );
    m_ac.add( "update_settings(" );
    m_ac.add( "quit()" );
    m_ac.add( "invert()" );
  }

  wxString Complete( const wxString& str ){
    m_index = 0;
    m_words = m_ac.match( std::string(str) );
    return ( m_words.size() == 0 ? str : m_words.get(0) );
  }

  void Forget(){
    m_words.clear();
  }

  wxString Get() const{
    if ( m_words.size() == 0 ){
      return "";
    }
    return m_words.get(m_index);
  }

  bool Has() const{
    return m_words.size() > 0;
  }

  wxString Next(){
    assert( Has() );
    m_index = ( m_index + 1 ) % m_words.size();
    return m_words.get( m_index );
  }

  wxString Prev(){
    assert( Has() );
    m_index = ( m_index - 1 ) % m_words.size();
    return m_words.get( m_index );
  }
private:
  faint::AutoComplete m_ac;
  faint::Words m_words;
  size_t m_index;
};

DEFINE_EVENT_TYPE( EVT_PYTHON_COMMAND )
DEFINE_EVENT_TYPE( EVT_GOT_KEY )

InterpreterCtrl::InterpreterCtrl( wxWindow* parent )
: wxTextCtrl( parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_RICH ),
  m_liveFeedback(false)
{
  m_completion = new AutoCompleteState();
  m_fileCompletion = new FileAutoComplete();
  m_inputStart = 0;
  m_getKey = false;
  m_currRowStart = 0;
  m_historyIndex = 0;

  wxFont font( 10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
  wxColour bgColor( 255, 249, 189 );
  SetBackgroundColour( bgColor );
  SetDefaultStyle( wxTextAttr(wxColour(0, 0, 0), bgColor, font ) );

  WriteText("Faint Python Interpreter\n");
  SetFocus();
  const int pos = GetLastPosition();
  SetSelection( pos, pos );
}

InterpreterCtrl::~InterpreterCtrl(){
  delete m_completion;
  delete m_fileCompletion;
}

void InterpreterCtrl::AddText( const wxString& text ){
  SetInsertionPoint( GetLastPosition() );
  AppendText( "\n");
  AppendText( text );
}

void InterpreterCtrl::BackwardWord(){
  long currPos = GetInsertionPoint();
  wxString text = GetRange( m_currRowStart, currPos );
  std::cout << text << std::endl;
  size_t pos = text.rfind( " " );
  std::cout << " ->" << pos << std::endl;
  if ( pos != wxString::npos ){
    SetInsertionPoint( currPos - ( text.size() - pos ) );
  }
  else {
    SetInsertionPoint( m_inputStart );
  }
}

long InterpreterCtrl::BeginningOfLine() const{
  long curr = GetInsertionPoint();
  long x, y;
  PositionToXY( curr, &x, &y );
  return( curr - x + 4 );
}

void InterpreterCtrl::CommandCompletion( bool shiftHeld ){
  // Python command completion.
  // Search from the end...
  int fromPos = GetLastPosition();
  if ( m_completion->Has() ){
    // ...unless we've completed before, if so
    // search from the completion-start
    fromPos -= m_completion->Get().size();
  }

  wxString text = GetRange( m_inputStart, fromPos );
  size_t pos = text.find_last_of( " (." );
  if ( pos != wxString::npos ){
    text = text.substr( pos + 1, text.size() - pos );
  }
  else {
    pos = 0;
  }

  wxString adjust = m_completion->Has()?
    ( shiftHeld ?
        m_completion->Prev() :
      m_completion->Next() ) :
    m_completion->Complete( text );
  Remove( m_inputStart + ( pos == 0 ? 0 : pos + 1 ), GetLastPosition() );
  AppendText( adjust );
}

void InterpreterCtrl::DeleteSelection() {
  long start, end;
  GetSelection(&start,&end );
  Remove( start, end );
}

bool InterpreterCtrl::Editable( long pos ) const{
  return !ReadOnly( pos );
}

long InterpreterCtrl::EndOfLine() const{
  long x, y;
  long curr = GetInsertionPoint();
  PositionToXY( curr, &x, &y );
  long len = GetLineLength( y );
  return curr + len - x;
}

void InterpreterCtrl::ForwardWord(){
  long currPos = GetInsertionPoint();
  wxString text = GetRange( currPos, GetLastPosition() );
  size_t spacePos = text.find( " " );
  if ( spacePos != wxString::npos ){
    SetInsertionPoint( currPos + spacePos + 1 );
  }
  else {
    SetInsertionPoint( GetLastPosition() );
  }
}

void InterpreterCtrl::GetKey(){
  m_getKey = true;
}

void InterpreterCtrl::GotKey( int keyCode, int modifiers ){
  wxCommandEvent event( EVT_GOT_KEY, GetId() );
  event.SetInt( keyCode );
  event.SetExtraLong( modifiers );
  GetEventHandler()->ProcessEvent( event );
}

bool InterpreterCtrl::HasSelection() const{
  long start, end;
  GetSelection( &start, &end );
  return start != end;
}

void InterpreterCtrl::NewContinuation(){
  int lastPos = GetLastPosition();
  SetInsertionPoint( lastPos );
  AppendText("... ");
  m_inputStart = GetLastPosition();
  AppendText( "  " );
}

void InterpreterCtrl::NewPrompt(){
  int lastPos = GetLastPosition();
  SetInsertionPoint( lastPos );

  AppendText( m_getKey ? "[ press key ]" : ">>> " );
  m_inputStart = GetLastPosition();
  m_currRowStart = m_inputStart;
}

void InterpreterCtrl::OnChar( wxKeyEvent& event ){
  // The OnChar handler is used for characters (normal text entry ).
  // wxWidgets EVT_CHAR-handlers receives keycodes with case depending on
  // the shift key, and further modified by ctrl etc.

  int keyCode = event.GetKeyCode();
  m_completion->Forget();

  bool navigation =
    keyCode == WXK_UP ||
    keyCode == WXK_DOWN ||
    keyCode == WXK_RIGHT ||
    keyCode == WXK_LEFT ||
    keyCode == WXK_END ||
    keyCode == WXK_HOME;

  if ( navigation ){
    event.Skip();
    return;
  }

  if ( GetInsertionPoint() < m_inputStart ){
    // Don't allow editing in read only regions
    SetInsertionPoint( m_inputStart );
    return;
  }

  else if ( keyCode == WXK_RETURN ){
    #ifndef __WXMSW__
    AppendText("\n");
    #endif
    PushLine();
    return;
  }
  else {
    // SetDefaultStyle does not affect normal text entry (i.e. what would happen on
    // event.Skip(). (There might be some better way to do this)
    wxChar c( keyCode );
    WriteText( wxString(c) );
  }
}

void InterpreterCtrl::OnKeyDown( wxKeyEvent& event ){
  // The OnKeyDown handler is used for commands (e.g. ctrl+c etc.), as
  // opposed to the OnChar handler.
  // Note: wxWidgets EVT_KEY_DOWN-handlers receive the keycode (e.g. for 'a') as
  // an uppercase letter ('A') - modifiers can be retrieved separately from the KeyEvent

  int keyCode = event.GetKeyCode();
  if ( m_getKey ){
    // Retrieve the keypress which the interpreter has requested from the user
    // (e.g. for a key bind or a yes/no query)
    if ( keyCode == WXK_CONTROL || keyCode == WXK_ALT || keyCode == WXK_SHIFT ){
      // Modifier keys are relevant, but should not complete the
      // key-query.
      return;
    }

    m_getKey = false;
    int modifiers = python_bind_modifiers(event);
    GotKey( keyCode, modifiers );
    AppendText("\n");
    NewPrompt();
    return;
  }

  if ( m_liveFeedback && keyCode == wxChar('.') ){
    wxString text = GetRange( BeginningOfLine(), GetLastPosition() );
    faint::resolve_object(std::string(text));
  }

  if ( keyCode == WXK_RETURN ){
    if ( event.ControlDown() ){
      // Ignore enter when control held as this outputs some garbage
      // character. Attempts to change this with Set[Raw]ControlDown
      // didn't help much.
      return;
    }
    // Move to end of line on enter before inserting line break
    // and comitting

    wxString text = GetRange( BeginningOfLine(), GetLastPosition() );
    if ( !text.empty() ){
      m_history.push_back(text);
    }
    m_historyIndex = m_history.size();

    if ( GetInsertionPoint() != GetLastPosition() ){
      SetInsertionPoint( GetLastPosition() );
    }
    event.Skip();
    return;
  }

  // Fixme: Also handled in the frame? (F8, but not escape)
  if ( keyCode == WXK_F8 || keyCode == WXK_ESCAPE ){
    GetParent()->Close();
    return;
  }

  if ( keyCode == WXK_TAB ){
    if ( in_quote( GetRange( m_inputStart, GetLastPosition() ) ) ) {
      PathCompletion( event.ShiftDown() );
    }
    else {
      CommandCompletion( event.ShiftDown() );
    }
    return;
  }

  if ( keyCode != WXK_SHIFT ){
    // Forget autocompletion state when a new entry is performed
    // (shift is ignored to support shift+tab, and it does not
    // cause an entry)
    m_fileCompletion->Forget();
    m_completion->Forget();
  }

  if ( keyCode == WXK_BACK ){
    if ( GetInsertionPoint() > m_inputStart ){
      Remove( GetInsertionPoint() - 1, GetInsertionPoint() );
    }
    return;
  }
  else if ( keyCode == WXK_DELETE ){
    if ( HasSelection() ){
      if ( SelectionEditable() ){
        DeleteSelection();
      }
    }
    else {
      // No selection, remove at insertion point
      if ( Editable( GetInsertionPoint() ) ) {
        Remove( GetInsertionPoint(), GetInsertionPoint() + 1 );
      }
    }
    return;
  }
  else if ( keyCode == WXK_HOME ){
    const long insertionPoint = GetInsertionPoint();
    SetInsertionPoint( m_inputStart );
    if ( event.ShiftDown() ){
      SetSelection( insertionPoint, m_inputStart );
    }
    return;
  }
  else if ( event.AltDown() || ( wxMOD_ALTGR == ( event.GetModifiers() & wxMOD_ALTGR ) ) ){
    if ( keyCode == wxChar('K') ){
      kill_line( *this );
      return;
    }
    else if ( keyCode == wxChar('A' ) ){
      ToBeginningOfLine();
      return;
    }
    else if ( keyCode == wxChar('E') ){
      ToEndOfLine();
      return;
    }
    else if ( keyCode == wxChar('F')){
      ForwardWord();
      return;
    }
    else if ( keyCode == wxChar('B')){
      BackwardWord();
      return;
    }
    // For some reason "[" and "]" stopped working in msw after I added the EVT_KEY_DOWN handler
    // (regardless of Skip())
    else if ( keyCode == wxChar('8') ){
      WriteText( wxString("[") );
      return;
    }
    else if ( keyCode == wxChar('9') ){
      WriteText( wxString("]") );
      return;
    }
    else {
      event.Skip();
    }
  }
  else if ( event.ControlDown() ){
    if ( keyCode == wxChar('F') ){
      forward_char( *this );
    }
    else if ( keyCode == wxChar('K') ){
      kill_line( *this );
    }
    else if ( keyCode == wxChar('A') ){
      ToBeginningOfLine();
    }
    else if ( keyCode == wxChar('E') ){
      ToEndOfLine();
    }
    else if ( keyCode == wxChar('B') ){
      backward_char(*this);
    }
    else if ( keyCode == wxChar('C') ){
      Copy();
    }
    else if ( keyCode == wxChar('V') ){
      if ( SelectionEditable() ){
        Paste();
      }
    }
    else if ( keyCode == wxChar('X') ){
      if ( SelectionEditable() ){
        Cut();
      }
    }
    else if ( keyCode == WXK_UP ||keyCode == WXK_DOWN ){
      // Up and down navigates the command history

      if ( m_history.empty() || !Editable(GetInsertionPoint()) ){
        return;
      }

      if ( keyCode == WXK_DOWN ){
        m_historyIndex = std::min( m_historyIndex + 1, m_history.size() );
        if ( m_historyIndex == m_history.size() ){
          return;
        }
      }
      else if ( keyCode == WXK_UP && m_historyIndex != 0 ){
        m_historyIndex -= 1;
      }

      Freeze();
      ToBeginningOfLine();
      kill_line(*this);
      WriteText(m_history[m_historyIndex]);
      Thaw();
    }
    return;
  }

  // Allow the key event to reach the character handler
  event.Skip();
}

void InterpreterCtrl::PathCompletion( bool shiftDown ){
  wxString text = GetRange( m_inputStart, GetLastPosition() );
  size_t pos = text.rfind( "\"" );
  if ( pos != wxString::npos ){
    text = text.substr( pos + 1, text.size() - pos );
  }
  else {
    pos = 0;
  }

  wxString adjust = m_fileCompletion->Has() ? ( shiftDown ? m_fileCompletion->Previous() : m_fileCompletion->Next() ) :
    m_fileCompletion->Complete( std::string(text) );

  Remove( m_inputStart + pos + 1, GetLastPosition() );
  AppendText( adjust );
}

void InterpreterCtrl::PushLine(){
  wxCommandEvent event( EVT_PYTHON_COMMAND, GetId() );
  wxString text = GetRange( m_inputStart, GetLastPosition() );
  event.SetString( text );
  GetEventHandler()->ProcessEvent( event );
}

bool InterpreterCtrl::ReadOnly( long pos ) const{
  return pos < m_inputStart;
}

bool InterpreterCtrl::SelectionEditable() const{
  long start, end;
  GetSelection( &start, &end );
  if ( start == end ){
    return false;
  }
  if ( ReadOnly( start ) || ReadOnly( end ) ){
    return false;
  }
  return true;
}

void InterpreterCtrl::ToBeginningOfLine(){
  SetInsertionPoint( BeginningOfLine() );
}

void InterpreterCtrl::ToEndOfLine(){
  SetInsertionPoint( EndOfLine() );
}

BEGIN_EVENT_TABLE( InterpreterCtrl, wxTextCtrl )
EVT_KEY_DOWN( InterpreterCtrl::OnKeyDown )
EVT_CHAR( InterpreterCtrl::OnChar )
END_EVENT_TABLE()
