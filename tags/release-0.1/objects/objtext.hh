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
#include "object.hh"
#include "textbuffer.hh"
#include "commands/command.hh"
#include "util/split_string.hh"

extern const std::string s_TypeText;

class wxString;
FaintSettings GetTextSettings();

class ObjText : public Object{
public:
  ObjText( const Rect&, const wxString&, const FaintSettings& );
  ObjText( const ObjText& );
  Object* Clone() const;
  void Draw( FaintDC& );
  void DrawMask( FaintDC& );
  IntRect GetRefreshRect();
  std::vector<Point> GetResizePoints();
  bool HitTest( const Point& );

  // Non-virtual
  void AnchorMiddle();
  Line GetCaret() const;
  Rect GetAutoSizedRect() const;
  const wxString& GetString();
  TextBuffer* GetTextBuffer();
  faint::coord LineSpacing() const;
  faint::coord RowHeight() const;
  void SetEdited( bool );
private:
  void Init();
  Line ComputeCaret(const FaintDC&, const Tri&, faint::coord, const text_lines_t& );
  TextBuffer m_textBuf;
  bool m_beingEdited;
  Line m_caret;
  mutable faint::coord m_rowHeight;
  mutable int m_lastFontSize;
  mutable std::string m_lastFontFace;
  FaintSettings m_highlightSettings;
};

class TextEntryCommand : public Command {
public:
  TextEntryCommand( ObjText*, const wxString& newText, const wxString& oldText );
  void Do( faint::Image& );
  void Undo( faint::Image& );
private:
  ObjText* m_textObj;
  const wxString m_old;
  const wxString m_new;
};

text_lines_t split_text_buffer( const TextBuffer&, faint::coord width, const FaintSettings& );
Command* AutoCropText( ObjText* object );

#endif
