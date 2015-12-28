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

#include "textbuffer.hh"

const wxChar TextBuffer::eol('\n');

TextBuffer::TextBuffer(){
  m_caret = 0;
  m_sel.active = false;
  m_sel.origin = 0;
  m_sel.end = 0;
}

TextBuffer::TextBuffer( const wxString& text ){
  m_caret = 0;
  m_sel.active = false;
  m_sel.origin = 0;
  m_sel.end = 0;
  m_data = text;
}

size_t TextBuffer::caret() const{
  return m_caret;
}

void TextBuffer::caret( size_t pos, bool select ){
  if ( !m_sel.active ){
      m_sel.origin = m_caret;
  }
  m_sel.active = select;
  m_caret = m_sel.end = std::min( m_data.size(), pos );
}

void TextBuffer::advance( bool select ){
  if ( !m_sel.active ){
    m_sel.origin = m_caret;
  }
  m_sel.active = select;
  m_caret = m_sel.end = std::min( m_caret + 1, m_data.size() );
}

void TextBuffer::devance( bool select ){
  if ( !m_sel.active ){
    m_sel.origin = m_caret;
  }
  m_sel.active = select;

  if ( m_caret == 0 ){
    return;
  }
  m_caret = m_sel.end = m_caret - 1;
}

void TextBuffer::move_up( bool select ){
  size_t currLineStart = prev(eol);
  if ( currLineStart == 0 ){
    // Already on the first line, move to start of text
    caret((0), select);
    return;
  }
  size_t x = m_caret - currLineStart;
  size_t prevLineStart = prev( eol, currLineStart );
  caret(std::min(prevLineStart + x, currLineStart), select );
}

void TextBuffer::move_down( bool select ){
  size_t currLineStart = prev(eol);
  size_t x = m_caret - currLineStart;
  size_t pos = next(eol);
  caret(std::min( pos + x, next(eol, pos + 1)),
    select);
}

void TextBuffer::del(){
  if ( m_sel.active ){
    del_selection();
  }
  else {
    if ( m_data.size() > m_caret ){
      m_data.erase( m_caret, 1 );
    }
  }
}

void TextBuffer::del_back(){
  if ( m_sel.active ){
    del_selection();
  }
  else if ( m_caret == 0 ) {
    return;
  }
  else {
    m_caret--;
    del();
  }
}

void TextBuffer::del_selection(){
  if ( ! m_sel.active ){
    return;
  }
  m_data.erase( m_sel.min(), m_sel.num() );
  m_caret = m_sel.min();
  m_sel.active = false;
  return;
}

void TextBuffer::select_none(){
  m_sel.active = false;
}

void TextBuffer::insert( const wxChar& c ){
  del_selection();
  m_data.insert( m_caret, 1, c );
  m_caret+=1;
}

void TextBuffer::insert( const wxString& str ){
  del_selection();
  m_data.insert( m_caret, str );
  m_caret += str.size();
}

text_rng TextBuffer::get_sel_range() const{
  return m_sel.active ? std::make_pair( m_sel.min(), m_sel.max() )
    : std::make_pair(0,0);
}

wxString TextBuffer::get_selection() const{
  return m_sel.active ?
    m_data.substr( m_sel.min(), m_sel.num() ) :
    "";
}

void TextBuffer::clear(){
  m_data = "";
  m_caret = 0;
  m_sel.active = false;

}

const wxString& TextBuffer::get() const{
  return m_data;
}

size_t TextBuffer::size() const{
  return m_data.size();
}

size_t TextBuffer::next( const wxChar& c ) const{
  return next( c, m_caret );
}

size_t TextBuffer::prev( const wxChar& c ) const{
  return prev(c, m_caret );
}

size_t TextBuffer::next( const wxChar& c, size_t pos ) const{
  size_t found = m_data.find( c, pos );
  if ( found == wxString::npos ){
    return m_data.size();
  }
  return found;
}

size_t TextBuffer::prev( const wxChar& c, size_t pos ) const{
  size_t found = m_data.rfind( c, pos - 1 );
  if ( found == wxString::npos ){
    return 0;
  }
  return found;
}
