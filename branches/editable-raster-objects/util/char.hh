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

#ifndef FAINT_CHAR_HH
#define FAINT_CHAR_HH
#include <string>

namespace faint {
  class utf8_char{
  public:
    utf8_char();
    explicit utf8_char( const std::string&);
    explicit utf8_char( unsigned int codePoint );
    unsigned int code_point() const;
    size_t bytes() const;
    std::string str() const;
    bool operator==(const utf8_char&) const;
    bool operator!=(const utf8_char&) const;
  private:
    std::string m_char;
  };
  extern const utf8_char comma;
  extern const utf8_char downwards_arrow_with_tip_leftwards;
  extern const utf8_char eol;
  extern const utf8_char euro_sign;
  extern const utf8_char exclamation_mark;
  extern const utf8_char full_stop;
  extern const utf8_char hyphen;
  extern const utf8_char left_parenthesis;
  extern const utf8_char no_break_space;
  extern const utf8_char pilcrow_sign;
  extern const utf8_char question_mark;
  extern const utf8_char right_parenthesis;
  extern const utf8_char shown_eol;
  extern const utf8_char snowman;
  extern const utf8_char space;

  class utf8_string{
  public:
    static const size_t npos;
    utf8_string();
    utf8_string( size_t n, const faint::utf8_char& );
    explicit utf8_string( const std::string& );
    utf8_string substr( size_t pos, size_t n=std::string::npos) const;
    const std::string& str() const;
    const char* c_str() const;
    void clear();
    size_t size() const;
    utf8_string& erase( size_t, size_t n=npos);
    bool empty() const;
    size_t find( const faint::utf8_char&, size_t start ) const;
    size_t rfind( const faint::utf8_char&, size_t start ) const;
    utf8_string& insert( size_t, const utf8_string& );
    utf8_string& insert( size_t, size_t, const faint::utf8_char& );
    bool operator==( const utf8_string& ) const;
    utf8_string& operator+=( const faint::utf8_char& );
    utf8_string& operator+=( const faint::utf8_string& );
    utf8_string& operator=( const faint::utf8_string& );
    faint::utf8_char operator[](size_t) const;
  private:
    std::string m_data;
  };

  utf8_string operator+( const utf8_string&, const faint::utf8_char& );
  utf8_string operator+( const utf8_string&, const utf8_string& );

  bool is_punctuation(const utf8_char& );
}

#endif
