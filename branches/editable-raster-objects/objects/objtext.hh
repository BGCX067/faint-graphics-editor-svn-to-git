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

#ifndef FAINT_OBJTEXT_HH
#define FAINT_OBJTEXT_HH
#include <string>
#include "commands/command.hh"
#include "objects/object.hh"
#include "util/split_string.hh"
#include "util/textbuffer.hh"

extern const std::string s_TypeText;

class ObjText : public Object{
public:
  ObjText( const Rect&, const faint::utf8_string&, const Settings& );
  Object* Clone() const override;
  void Draw( FaintDC& ) override;
  void DrawMask( FaintDC& ) override;
  IntRect GetRefreshRect() override;
  bool ShowSizeBox() const override;

  // Non virtual
  // The distance from the top-left (quoi?) to the base-line
  faint::coord BaselineOffset() const;
  size_t CaretPos( const Point& ) const;
  LineSegment GetCaret() const;
  Rect GetAutoSizedRect() const;
  faint::utf8_string GetString() const;
  TextBuffer* GetTextBuffer();
  bool HasSelectedRange() const;
  faint::coord RowHeight() const;
  void SetCaretPos( size_t, bool select );
  void SetEdited( bool );
private:
  ObjText( const ObjText& ); // For Clone
  void Init();
  LineSegment ComputeCaret(const FaintDC&, const Tri&, faint::coord, const text_lines_t& );
  TextBuffer m_textBuf;
  bool m_beingEdited;
  LineSegment m_caret;
  mutable faint::coord m_rowHeight;
  mutable int m_lastFontSize;
  mutable std::string m_lastFontFace;
  Settings m_highlightSettings;
};

text_lines_t split_text_buffer( const TextBuffer&, const Settings&, const max_width_t& );
Command* crop_text_region_command( ObjText* object );

#endif
