// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#include <sstream>
#include "app/canvas.hh"
#include "app/get-app-context.hh"
#include "commands/calibrate-cmd.hh"
#include "geo/line.hh"
#include "geo/measure.hh"
#include "geo/geo-func.hh"
#include "geo/size.hh"
#include "rendering/overlay.hh"
#include "tasks/task.hh"
#include "text/text-buffer.hh"
#include "tools/tool.hh" // Fixme: For TextContext et al., move to new file
#include "util/image.hh"
#include "util/pos-info.hh"
#include "util/text-entry-util.hh"
#include "util-wx/key-codes.hh"

namespace faint{

static Optional<std::pair<coord, utf8_string>> get_entered_measure(const TextBuffer& text){
  const utf8_string& s(text.get());
  if (!is_ascii(s)){
    return no_option();
  }

  std::stringstream ss(s.str());
  coord value = -1;
  ss >> value;
  if (value <= 0){
    return no_option();
  }

  std::string unit;
  ss >> unit;
  if (unit.size() == 0){
    return no_option();
  }

  return option(std::make_pair(value, utf8_string(unit)));
}

class CalibrateEnterMeasure : public Task,
                              public TextContext,
                              public SelectionContext
{
  // Reads measure input for the line and shown the entry in an
  // edit-overlay.
public:
  CalibrateEnterMeasure(const LineSegment& line, const PosInfo& info)
    : m_active(true),
      m_line(line)
  {
    assert(length(m_line) != 0);
    info.status.SetMainText("Enter line length");
    get_app_context().BeginTextEntry();
  }

  ~CalibrateEnterMeasure() override{
    if (m_active){
      get_app_context().EndTextEntry();
      m_active = false;
    }
  }

  void Activate() override{
  }

  bool AcceptsPastedText() const override{
    return true;
  }

  bool EatsSettings() const override{
    return false;
  }

  bool RefreshOnMouseOut() const override{
    return true;
  }

  TaskResult Char(const KeyInfo& info) override{
    if (info.key.Is(key::esc)){
      // Cancel on escape
      return TaskResult::CHANGE;
    }
    else if (info.key.Is(key::enter)){
      // Commit on enter, if valid text
      coord len = length(m_line);
      assert(len != 0);

      return get_entered_measure(m_text).Visit(
        [&](std::pair<coord, const utf8_string&> entry){
          info.status.SetMainText("");
          if (m_active){
            get_app_context().EndTextEntry();
            m_active = false;
          }

          // Fixme: mm hard-coded, support other units
          m_command.Set(calibrate_command(Calibration(m_line, entry.first,
            entry.second), info.canvas.GetImage().GetCalibration()));
          return TaskResult::COMMIT_AND_CHANGE;
        },

        [](){
          return TaskResult::NONE;
        });
    }
    else if (handle_key_press(info.key, m_text)){
      return TaskResult::DRAW;
    }
    return TaskResult::NONE;
  }

  Optional<utf8_string> CopyText() const override{
    return m_text.get_selection();
  }

  Optional<utf8_string> CutText() override{
    Optional<utf8_string> s(m_text.get_selection());
    m_text.del();
    return s;
  }

  bool Delete() override{
    m_text.del();
    return true;
  }

  bool Deselect() override{
    m_text.select_none();
    return true;
  }

  TaskResult DoubleClick(const PosInfo& pos) override{
    // Todo: Select double-clicked word
    return MouseDown(pos);
  }

  void Draw(FaintDC&, Overlays& overlays, const PosInfo&) override{
    if (m_active){
      overlays.Line(m_line);
      overlays.Textbox(mid_point(m_line), m_text, "Distance");
    }
  }

  bool DrawBeforeZoom(Layer) const override{
    return false;
  }

  Command* GetCommand() override{
    return m_command.Take();
  }

  Cursor GetCursor(const PosInfo&) const override{
    return Cursor::CARET; // Fixme: Only when over textbox
  }

  Task* GetNewTask() override{
    return nullptr;
  }

  IntRect GetRefreshRect(const RefreshInfo&) const override{
    return IntRect(IntPoint(0,0),IntSize(640,480)); // Fixme
  }

  Optional<const faint::HistoryContext&> HistoryContext() const override{
    return {};
  }

  TaskResult MouseDown(const PosInfo&) override{
    return TaskResult::NONE;
  }

  TaskResult MouseUp(const PosInfo&) override{
    return TaskResult::NONE;
  }

  TaskResult MouseMove(const PosInfo&) override{
    return TaskResult::NONE;
  }

  void Paste(const utf8_string& str) override{
    m_text.insert(str);
  }

  TaskResult Preempt(const PosInfo&) override{
    return TaskResult::NONE;
  }

  Optional<const faint::SelectionContext&> SelectionContext() const override{
    return Optional<const faint::SelectionContext&>(*this);
  }

  bool SelectAll() override{
    m_text.select(m_text.all());
    return true;
  }

  Optional<const faint::TextContext&> TextContext() const override{
    return Optional<const faint::TextContext&>(*this);
  }

  void SelectionChange() override{}

  void SetLayer(Layer) override{}

  void UpdateSettings() override{}

private:
  bool m_active;
  PendingCommand m_command;
  LineSegment m_line;
  TextBuffer m_text;
};

Task* calibrate_enter_measure(const LineSegment& line, const PosInfo& info){
  return new CalibrateEnterMeasure(line, info);
}

}
