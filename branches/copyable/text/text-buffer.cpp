// -*- coding: us-ascii-unix -*-
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
#include "geo/primitive.hh"
#include "text/char-constants.hh"
#include "text/text-buffer.hh"
#include "text/utf8-string.hh"

namespace faint{

TextBuffer::TextBuffer()
  : m_caret(0)
{
  m_sel.active = false;
  m_sel.origin = 0;
  m_sel.end = 0;
}

TextBuffer::TextBuffer( const faint::utf8_string& text )
  : m_data(text),
    m_caret(0)
{
  m_sel.active = false;
  m_sel.origin = 0;
  m_sel.end = 0;
}

void TextBuffer::advance( bool select ){
  if ( !m_sel.active ){
    m_sel.origin = resigned(m_caret);
  }
  m_sel.active = select;
  m_sel.end = resigned(std::min( m_caret + 1, m_data.size() ));
  m_caret = to_size_t(m_sel.end);
}

faint::utf8_char TextBuffer::at( size_t pos ) const{
  assert( pos < m_data.size() );
  return m_data[pos];
}

size_t TextBuffer::caret() const{
  return m_caret;
}

void TextBuffer::caret( size_t pos, bool select ){
  if ( !m_sel.active ){
    m_sel.origin = resigned(m_caret);
  }
  m_sel.active = select;
  m_sel.end = resigned(std::min( m_data.size(), pos ));
  m_caret = to_size_t(m_sel.end);
}

void TextBuffer::clear(){
  m_data.clear();
  m_caret = 0;
  m_sel.active = false;
}

void TextBuffer::del(){
  if ( m_sel.active ){
    del_selection();
  }
  else {
    if ( m_data.size() > m_caret ){
      m_data.erase(m_caret,1);
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
  m_data.erase( to_size_t(m_sel.min()), to_size_t(m_sel.num()) );
  m_caret = to_size_t(m_sel.min());
  m_sel.active = false;
  return;
}

void TextBuffer::devance( bool select ){
  if ( !m_sel.active ){
    m_sel.origin = resigned(m_caret);
  }
  m_sel.active = select;

  if ( m_caret == 0 ){
    return;
  }
  m_sel.end = resigned(m_caret - 1);
  m_caret = to_size_t(m_sel.end);
}

const faint::utf8_string& TextBuffer::get() const{
  return m_data;
}

text_rng TextBuffer::get_sel_range() const{
  return m_sel.active ? std::make_pair( m_sel.min(), m_sel.max() )
    : std::make_pair(0,0);
}

faint::utf8_string TextBuffer::get_selection() const{
  return m_sel.active ?
    m_data.substr( to_size_t(m_sel.min()), to_size_t(m_sel.num()) ) :
    faint::utf8_string("");
}

void TextBuffer::insert( const faint::utf8_char& c ){
  del_selection();
  m_data.insert( m_caret, 1, c );
  m_caret+=1;
}

void TextBuffer::insert( const faint::utf8_string& str ){
  del_selection();
  m_data.insert( m_caret, str );
  m_caret += str.size();
}

void TextBuffer::move_down( bool select ){
  size_t currLineStart = prev(faint::eol);
  size_t x = m_caret - currLineStart;
  size_t pos = next(faint::eol);
  caret(std::min( pos + x, next(faint::eol, pos + 1)),
    select);
}

void TextBuffer::move_up( bool select ){
  size_t currLineStart = prev(faint::eol);
  if ( currLineStart == 0 ){
    // Already on the first line, move to start of text
    caret(0, select);
    return;
  }
  size_t x = m_caret - currLineStart;
  size_t prevLineStart = prev( faint::eol, currLineStart );
  caret(std::min(prevLineStart + x, currLineStart), select );
}

void TextBuffer::select_none(){
  m_sel.active = false;
}

void TextBuffer::select( const text_rng& range ){
  assert(range.first < m_data.size() + 1);
  assert(range.second < m_data.size() + 1);
  m_sel.origin = resigned(range.first);
  m_sel.end = resigned(range.second);
  m_caret = range.second;
  m_sel.active = true;
}

size_t TextBuffer::size() const{
  return m_data.size();
}

size_t TextBuffer::next( const faint::utf8_char& c ) const{
  return next( c, m_caret );
}

size_t TextBuffer::next( const faint::utf8_char& c, size_t pos ) const{
  size_t found = m_data.find( c, pos );
  if ( found == faint::utf8_string::npos ){
    return m_data.size();
  }
  return found;
}

size_t TextBuffer::prev( const faint::utf8_char& c ) const{
  return prev(c, m_caret );
}

size_t TextBuffer::prev( const faint::utf8_char& c, size_t pos ) const{
  size_t found = m_data.rfind( c, pos - 1 );
  if ( found == faint::utf8_string::npos ){
    return 0;
  }
  return found;
}

bool word_delimiter( const faint::utf8_char& ch ){
  return faint::is_punctuation(ch) ||
    ch == faint::eol ||
    ch == faint::left_parenthesis ||
    ch == faint::right_parenthesis ||
    ch == faint::space;
}

text_rng word_boundaries(size_t pos, const TextBuffer& buffer ){
  if ( pos >= buffer.size() ){
    return text_rng(buffer.size(), buffer.size());
  }

  size_t left = 0;
  size_t right = buffer.size();
  if ( word_delimiter(buffer.at(pos) ) ){
    // Use the contiguous range of the delimiter at the given pos as
    // the boundaries.
    faint::utf8_char ch = buffer.at(pos);
    for ( size_t i = pos; i != 0; i-- ){
      if ( buffer.at(i) != ch ){
        left = i + 1;
        break;
      }
    }
    for ( size_t i = pos; i != buffer.size(); i++ ){
      if ( buffer.at(i) != ch ){
        right = i;
        break;
      }
    }
  }
  else {
    // Find the delimiters and return the range of characters between
    // them.
    for ( size_t i = pos; i != 0; i-- ){
      if ( word_delimiter( buffer.at(i) ) ){
        left = i + 1;
        break;
      }
    }
    for ( size_t i = pos; i != buffer.size(); i++ ){
      if ( word_delimiter(buffer.at(i) ) ){
        right = i;
        break;
      }
    }
  }
  return text_rng(left, right);
}

} // namespace
