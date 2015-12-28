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
#include "app/get-app-context.hh"
#include "commands/text-entry-cmd.hh"
#include "objects/objtext.hh"
#include "rendering/overlay.hh"
#include "tasks/text-edit.hh"
#include "tasks/text-select.hh"
#include "text/char-constants.hh"
#include "text/formatting.hh"
#include "util/command-util.hh"

namespace faint{

inline bool is_exit_key(const KeyPress& key){
  return key.Is(Ctrl, key::enter) ||
    key.Is(key::esc);
}

inline bool outside(Object* obj, const PosInfo& info){
  return !bounding_rect(obj->GetTri()).Contains(info.pos);
}

inline bool right_click(const PosInfo& info){
  return info.modifiers.RightMouse();
}

inline bool select(const Mod& mod){
  return mod.Shift();
}

static bool handle_special_key(const KeyPress& key, ObjText* textObject){
  TextBuffer& text = *(textObject->GetTextBuffer());
  if (key.Is(key::back)){
    text.del_back();
    return true;
  }
  else if (key.Is(key::del)){
    return true;
  }
  else if (key.Is(key::enter)){
    text.insert(eol);
    return true;
  }
  else if (key.Is(key::left)){
    text.devance(select(key.Modifiers()));
    return true;
  }
  else if (key.Is(key::right)){
    text.advance(select(key.Modifiers()));
    return true;
  }
  else if (key.Is(key::up)){
    text.move_up(select(key.Modifiers()));
    return true;
  }
  else if (key.Is(key::down)){
    text.move_down(select(key.Modifiers()));
    return true;
  }
  else if (key.Is(key::home)){
    size_t newPos = text.prev(eol);
    text.caret(newPos == 0 ? newPos : newPos + 1, select(key.Modifiers()));
    return true;
  }
  else if (key.Is(key::end)){
    text.caret(text.next(eol), select(key.Modifiers()));
    return true;
  }
  else if (key.Is(Ctrl, key::B)){
    textObject->Set(ts_FontBold, textObject->GetSettings().Not(ts_FontBold));
    return true;
  }
  else if (key.Is(Ctrl, key::I)){
    textObject->Set(ts_FontItalic, textObject->GetSettings().Not(ts_FontItalic));
    return TASK_DRAW;
  }
  return false;
}

static void select_word_at_pos(ObjText* textObject, const Point& pos){
  size_t caret = textObject->CaretPos(pos);
  TextBuffer* textBuffer = textObject->GetTextBuffer();
  textBuffer->select(word_boundaries(caret, *textBuffer));
}

class EditText : public Task {
public:

  EditText(const Rect& r, const utf8_string& str, Settings& settings)
    : m_active(false),
      m_newTextObject(true),
      m_settings(settings),
      m_textObject(new ObjText(r, str, settings))
  {}

  EditText(ObjText* obj, Settings& s)
    : m_active(false),
      m_newTextObject(false),
      m_oldText(obj->GetTextBuffer()->get()),
      m_settings(s),
      m_textObject(obj)
  {
    m_settings = m_textObject->GetSettings();
  }

  ~EditText(){
    if (m_active){
      EndEntry();
    }
  }

  void Activate() override{
    assert(!m_active);
    m_textObject->SetActive(true);
    m_textObject->SetEdited(true);
    TextBuffer* buf(m_textObject->GetTextBuffer());
    buf->caret(buf->size());
    get_app_context().BeginTextEntry();
    m_active = true;
  }

  bool AcceptsPastedText() const override{
    return true;
  }

  TaskResult Char(const KeyInfo& info) override{
    if (is_exit_key(info.key)){
      return Commit(info.layerType);
    }
    if (handle_special_key(info.key, m_textObject)){
      return TASK_DRAW;
    }
    m_textObject->GetTextBuffer()->insert(info.ch);
    return TASK_DRAW;
  }

  bool CopyText(utf8_string& str, const erase_copied& eraseCopied) override{
    str = m_textObject->GetTextBuffer()->get_selection();
    if (eraseCopied.Get() && !str.empty()){
      m_textObject->GetTextBuffer()->del();
    }
    return true;
  }

  TaskResult Delete() override{
    m_textObject->GetTextBuffer()->del();
    return TASK_DRAW;
  }

  TaskResult Deselect() override{
    m_textObject->GetTextBuffer()->select_none();
    return TASK_DRAW;
  }

  void Draw(FaintDC& dc, Overlays& overlays, const Point&) override{
    overlays.Corners(m_textObject->GetTri());
    if (m_newTextObject){
      m_textObject->Draw(dc);
    }
    if (!m_textObject->HasSelectedRange()){
      overlays.Caret(m_textObject->GetCaret());
    }
  }

  bool DrawBeforeZoom(Layer layer) const override{
    // This is only relevant when first editing raster text or creating
    // a new text object - when editing an existing object, the text
    // object draws itself, independently of the task.
    return layer == Layer::RASTER;
  }

  Command* GetCommand() override{
    return m_command.Retrieve();
  }

  Cursor GetCursor(const PosInfo& info) const override{
    if (m_textObject->GetTri().Contains(info.pos)){
      return Cursor::CARET;
    }
    return Cursor::ARROW;
  }

  Task* GetNewTask() override{
    return m_newTask.Retrieve();
  }

  IntRect GetRefreshRect(const IntRect&, const Point&) const override{
    return m_textObject->GetRefreshRect();
  }

  bool SupportsSelection() const override{
    return true;
  }

  TaskResult DoubleClick(const PosInfo& info) override{
    select_word_at_pos(m_textObject, info.pos);
    return TASK_DRAW;
  }

  TaskResult MouseDown(const PosInfo& info) override{
    assert(m_textObject != nullptr);
    if (right_click(info) || outside(m_textObject, info)){
      return Commit(info.layerType);
    }
    m_newTask.Set(select_text_task(m_textObject, m_newTextObject, info.pos));
    return TASK_PUSH;
  }

  TaskResult MouseUp(const PosInfo&) override{
    return TASK_NONE;
  }

  TaskResult MouseMove(const PosInfo& info) override{
    info.status.SetMainText("Use Ctrl+Enter to stop editing");
    info.status.SetText(str(info.pos));
    return TASK_NONE;
  }

  void Paste(const utf8_string& str) override{
    m_textObject->GetTextBuffer()->insert(str);
  }

  TaskResult Preempt(const PosInfo& info) override{
    return Commit(info.layerType);
  }

  TaskResult SelectAll() override{
    TextBuffer* text = m_textObject->GetTextBuffer();
    text->caret(0);
    text->caret(text->size(), true);
    return TASK_DRAW;
  }

  void UpdateSettings() override{
    m_textObject->UpdateSettings(m_settings);
  }

  EditText& operator=(const EditText&) = delete;
private:
  void EndEntry(){
    assert(m_active);

    get_app_context().EndTextEntry();
    if (m_textObject != nullptr){
      m_textObject->SetActive(false);
      m_textObject->SetEdited(false);
    }
    m_active = false;
  }

  TaskResult Commit(Layer layerType){
    if (m_active){
      EndEntry();
    }
    if (m_newTextObject){
      m_command.Set(add_or_draw(m_textObject, layerType));
      return TASK_COMMIT_AND_CHANGE;
    }

    const utf8_string& newText(m_textObject->GetTextBuffer()->get());
    if (newText == m_oldText){
      // Text unchanged, create no no command.
      return TASK_CHANGE;
    }
    m_command.Set(text_entry_command(m_textObject, New(newText), Old(m_oldText)));
    return TASK_COMMIT_AND_CHANGE;
  }

  bool m_active;
  PendingCommand m_command;
  PendingTask m_newTask;
  bool m_newTextObject;
  utf8_string m_oldText;
  Settings& m_settings;
  ObjText* m_textObject;
};

Task* edit_text_task(const Rect& r, const utf8_string& str, Settings& settings){
  return new EditText(r, str, settings);
}

Task* edit_text_task(ObjText* obj, Settings& settings){
  return new EditText(obj, settings);
}

}
