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

#ifndef FAINT_TEXTBUFFER_HH
#define FAINT_TEXTBUFFER_HH
#include "util/commonfwd.hh"
#include "util/char.hh"
#include <utility>

typedef std::pair<size_t, size_t> text_rng;

class TextBuffer{
public:
  TextBuffer();
  TextBuffer(const faint::utf8_string&);
  void advance( bool select=false );
  size_t caret() const;
  void caret(size_t pos, bool select=false);
  void clear();
  void del();
  void del_back();
  void devance( bool select=false );
  const faint::utf8_string& get() const;
  text_rng get_sel_range() const;
  faint::utf8_string get_selection() const;
  void insert( const faint::utf8_char& );
  void insert( const faint::utf8_string& );
  void move_down( bool select=false );
  void move_up( bool select=false );
  size_t next( const faint::utf8_char& ) const;
  size_t next( const faint::utf8_char&, size_t pos ) const;
  size_t prev( const faint::utf8_char& ) const;
  size_t prev( const faint::utf8_char&, size_t pos ) const;
  void select_none();
  size_t size() const;
private:
  void del_selection();
  struct{
    bool active;
    int origin;
    int end;

    inline int min() const{
      return std::min( origin, end );
    }
    inline int max() const{
      return std::max( origin, end );
    }
    inline int num() const{
      return max() - min();
    }
  } m_sel;

  faint::utf8_string m_data;
  size_t m_caret;
};

#endif
