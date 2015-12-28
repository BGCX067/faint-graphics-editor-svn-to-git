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
#include <cstdio>
#include <cstring>
#include "text/utf8-string.hh"

size_t charnum_to_byte(size_t, const char* utf8);
size_t num_characters(const char* utf8);
size_t bytes(unsigned char);
size_t byte_to_charnum(size_t, const char* utf8);

namespace faint{
  utf8_char::utf8_char( const std::string& utf8_data )
    : m_char(utf8_data)
  {}

  utf8_char::utf8_char( unsigned int cp ){
    const unsigned int continuation = 0x80;
    if ( cp <= 0x7f ){
      m_char += static_cast<char>((unsigned char)(cp));
    }
    else if ( cp <= 0x7ff ){
      const unsigned int lead_2 = 0xc0;
      m_char += static_cast<char>((unsigned char)((( cp >> 6) & 0x1f) | lead_2));
      m_char += static_cast<char>((unsigned char)((cp & 0x3f) | continuation));
    }
    else if ( cp <= 0xffff ){
      const unsigned int lead_3 = 0xe0;
      m_char += static_cast<char>((unsigned char)(((cp >> 12) & 0xf) | lead_3));
      m_char += static_cast<char>((unsigned char)(((cp >> 6) & 0x3f) | continuation));
      m_char += static_cast<char>((unsigned char)((cp & 0x3f) | continuation));
    }
    else if ( cp <= 0x1fffff ){
      const unsigned int lead_4 = 0xf0;
      m_char += static_cast<char>((unsigned char)(((cp >> 18) & 0xf) | lead_4));
      m_char += static_cast<char>((unsigned char)(((cp >> 12) & 0x3f) | continuation));
      m_char += static_cast<char>((unsigned char)(((cp >> 6 ) & 0x3f) | continuation));
      m_char += static_cast<char>((unsigned char)((cp & 0x3f) | continuation));
    }
    else {
      assert( false );
    }
  }

  size_t utf8_char::bytes() const{
    return m_char.size();
  }

std::string utf8_char::str() const{
  return m_char;
}

bool utf8_char::operator==( const utf8_char& other ) const{
  return m_char == other.m_char;
}

bool utf8_char::operator!=( const utf8_char& other ) const{
  return m_char != other.m_char;
}

utf8_string::utf8_string(){}

utf8_string::utf8_string( size_t n, const faint::utf8_char& ch){
  for ( size_t i = 0; i != n; i++ ){
    m_data += ch.str();
  }
}

utf8_string::utf8_string( const char* str )
  : m_data(str)
{}

utf8_string::utf8_string( const std::string& str )
  : m_data(str)
{}

size_t utf8_string::bytes() const{
  return m_data.size();
}

void utf8_string::clear() {
  m_data.clear();
}

utf8_string utf8_string::substr( size_t pos, size_t n ) const{
  size_t startByte = charnum_to_byte(pos, m_data.c_str());
  size_t numBytes = charnum_to_byte(pos + n, m_data.c_str()) - startByte;
  return utf8_string(m_data.substr(startByte, numBytes));
}

const char* utf8_string::c_str() const{
  return m_data.c_str();
}

const std::string& utf8_string::str() const{
  return m_data;
}

size_t utf8_string::size() const{
  return num_characters(m_data.c_str());
}

bool utf8_string::empty() const{
  return m_data.empty();
}

utf8_string& utf8_string::erase( size_t pos, size_t n ){
  size_t startByte = charnum_to_byte(pos, m_data.c_str());

  size_t numBytes = ( n == npos ? npos : charnum_to_byte(pos + n, m_data.c_str()) - startByte );
  m_data.erase( startByte, numBytes );
  return *this;
}

utf8_string& utf8_string::insert( size_t pos, const utf8_string& inserted ){
  m_data.insert( charnum_to_byte(pos, m_data.c_str()), inserted.str() );
  return *this;
}

utf8_string& utf8_string::insert( size_t pos, size_t num, const faint::utf8_char& c ){
  insert(pos, utf8_string(num, c));
  return *this;
}

faint::utf8_char utf8_string::operator[](size_t i) const{
  size_t pos = charnum_to_byte(i, m_data.c_str());
  size_t numBytes = ::bytes(static_cast<unsigned char>(m_data[pos]));
  return faint::utf8_char(m_data.substr(pos, numBytes));
}

size_t utf8_string::find( const faint::utf8_char& ch, size_t start ) const{
  // Fixme: Must step by full code points, not chars
  size_t pos = m_data.find(ch.str(), charnum_to_byte(start, m_data.c_str()));
  if ( pos == npos ){
    return pos;
  }
  return byte_to_charnum( pos, m_data.c_str() );
}

size_t utf8_string::rfind( const faint::utf8_char& ch, size_t start ) const{
  size_t pos = m_data.rfind(ch.str(), charnum_to_byte(start, m_data.c_str()));
  if ( pos == npos ){
    return pos;
  }
  return byte_to_charnum(pos, m_data.c_str());
}

utf8_string& utf8_string::operator=( const faint::utf8_string& other ){
  if ( &other == this ){
    return *this;
  }
  m_data = other.m_data;
  return *this;

}
utf8_string& utf8_string::operator+=( const faint::utf8_char& ch ){
  m_data += ch.str();
  return *this;
}

utf8_string& utf8_string::operator+=( const faint::utf8_string& str ){
  m_data += str.str();
  return *this;
}

utf8_string operator+( const utf8_string& str, const faint::utf8_char& ch ){
  return utf8_string(str.str() + ch.str());
}

utf8_string operator+( const utf8_string& lhs, const utf8_string& rhs ){
  return utf8_string(lhs.str() + rhs.str());
}

const size_t utf8_string::npos(std::string::npos);

bool is_ascii(const utf8_string& s){
  for ( size_t i = 0; i != s.size(); i++ ){
    if ( s[i].bytes() != 1 ){
      return false;
    }
  }
  return true;
}

std::ostream& operator<<(std::ostream& o, const faint::utf8_string& s){
  o << s.str();
  return o;
}

bool operator==( const utf8_string& lhs, const utf8_string& rhs ){
  return lhs.str() == rhs.str();
}

bool operator!=( const utf8_string& lhs, const utf8_string& rhs ){
  return !(lhs == rhs);
}

} // namespace

size_t bytes(unsigned char c){
  if ( (c & 0x80) == 0 ){
    return 1;
  }
  else if ( (c & 0x40) == 0 ){
    assert( false ); // Continuation
  }
  else if ( ( c & 0xe0 ) == 0xc0 ){
    return 2;
  }
  else if ( (c & 0xf0) == 0xe0 ){
    return 3;
  }
  else if ( ( c & 0xf8 ) == 0xf0 ){
    return 4;
  }
  assert( false );
  return 0;
}

size_t num_characters(const char* utf8){
  size_t num = 0;
  const unsigned char* ptr = (unsigned char*)utf8;
  while (*ptr != 0 ){
    ptr += bytes(*ptr);
    num++;
  }
  return num;
}

size_t byte_to_charnum( size_t byte, const char* data ){
  size_t curByte = 0;
  size_t charNum = 0;
  const unsigned char* ptr = (unsigned char*)data;

  while (*ptr!= 0 && curByte != byte ){
    size_t nBytes = bytes(*ptr);
    ptr += nBytes;
    curByte += nBytes;
    charNum++;
  }
  return charNum;
}

size_t charnum_to_byte( size_t caret, const char* data ){
  size_t charNum = 0;
  size_t offset = 0;
  const unsigned char* ptr = (unsigned char*)data;

  while ( *ptr != 0 && charNum != caret){
    size_t nBytes = bytes(*ptr);
    ptr += nBytes;
    offset += nBytes;
    charNum++;
  }
  return offset;
}

size_t bytes( size_t pos, const char* data ){
  size_t offset = charnum_to_byte(pos, data);
  return bytes(static_cast<unsigned char>(data[offset]));
}
