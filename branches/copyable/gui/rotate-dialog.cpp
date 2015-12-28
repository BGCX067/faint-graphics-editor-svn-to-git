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

#include <functional>
#include "wx/bmpbuttn.h"
#include "wx/dialog.h"
#include "wx/panel.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"
#include "app/get-app-context.hh"
#include "geo/measure.hh"
#include "gui/layout.hh"
#include "gui/placement.hh"
#include "gui/rotate-dialog.hh"
#include "text/formatting.hh"
#include "util/apply-target.hh"
#include "util/canvas.hh"
#include "util/command-util.hh"
#include "util/context-commands.hh"
#include "util/convert-wx.hh"
#include "util/gui-util.hh"
#include "util/object-util.hh"

namespace faint{

typedef std::vector<AcceleratorEntry> Accelerators;
static Command* dummy_function(const Canvas&){
  // Avoids leaving RotateDialog::m_cmdFunc uninitialized
  return nullptr;
}

typedef std::function<decltype(dummy_function)> cmd_func;
typedef std::function<void(wxCommandEvent&)> cmd_event_func;

static wxString get_rotate_target_name(const Canvas& canvas){
  return dispatch_target(get_apply_target(canvas),
    [&](OBJECT_SELECTION){
      return to_wx(get_collective_name(canvas.GetObjectSelection()));
    },
    [](RASTER_SELECTION){
      return wxString("Selection");
    },
    [](IMAGE){
      return wxString("Image");
    });
}

class RotateChoicePanel : public wxPanel{
public:
  RotateChoicePanel(wxWindow* parent, const ArtContainer& art, cmd_event_func horizontal, cmd_event_func vertical, cmd_event_func rotate, cmd_event_func level)
    : wxPanel(parent, wxID_ANY),
      m_cancelButton(nullptr),
      m_rotateButton(nullptr)
  {
    Accelerators acc;
    wxBoxSizer* rows = new wxBoxSizer(wxVERTICAL);
    CreateButtonRow(art, rows, acc, horizontal, vertical, rotate);
    rows->AddSpacer(item_spacing);
    CreateLevelRow(art, acc, rows, level);

    SetSizerAndFit(rows);
    SetAcceleratorTable(accelerators(acc));
  }

  void CreateLevelRow(const ArtContainer& art, Accelerators& acc, wxSizer* rows, cmd_event_func level){
    wxBoxSizer* levelRow = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* label = new wxStaticText(this, wxID_ANY, "&Horizontal level");

    wxBitmapButton* levelButton = new wxBitmapButton(this,
      wxID_ANY, art.Get(Icon::TOOL_LEVEL));
    levelButton->SetToolTip("Horizontal level tool");
    levelButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, level);
    levelRow->Add(levelButton);
    levelRow->Add(label, 0, wxALIGN_CENTER_VERTICAL);
    rows->Add(levelRow, 0, wxLEFT|wxRIGHT|wxDOWN, panel_padding);

    Bind(wxEVT_COMMAND_MENU_SELECTED, level, (int)key::H);
    acc.push_back({key::H, (int)key::H});
    acc.push_back({Alt+key::H, (int)key::H});
  }

  void CreateButtonRow(const ArtContainer& art, wxSizer* rows, Accelerators& acc, cmd_event_func horizontal, cmd_event_func vertical, cmd_event_func rotate){
    wxBoxSizer* buttonRow = new wxBoxSizer(wxHORIZONTAL);

    CreateButton("Flip Horizontally",
      art.Get(Icon::ROTATE_DIALOG_FLIP_HORIZONTAL),
      acc, key::Q,
      buttonRow, horizontal);

    CreateButton("Flip Vertically",
      art.Get(Icon::ROTATE_DIALOG_FLIP_VERTICAL),
      acc, key::W,
      buttonRow, vertical);

    m_rotateButton = CreateButton("Rotation...",
      art.Get(Icon::ROTATE_DIALOG_ROTATE_NEXT),
      acc, key::E,
      buttonRow, rotate);

    m_cancelButton = CreateCancelButton(buttonRow);
    rows->Add(buttonRow, 0, wxLEFT|wxRIGHT|wxUP, panel_padding);
  }

  wxButton* CreateButton(const wxString& toolTip, const wxBitmap& bmp, Accelerators& acc, const Key& keyCode, wxSizer* sizer, cmd_event_func cmdEvtFunc){
    // Create and add the button
    wxButton* btn = new wxBitmapButton(this, wxID_ANY, bmp);
    btn->SetInitialSize(to_wx(big_button_size));
    btn->SetToolTip(toolTip);
    sizer->Add(btn);

    btn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, cmdEvtFunc);
    Bind(wxEVT_COMMAND_MENU_SELECTED, cmdEvtFunc, keyCode);
    acc.push_back({keyCode, (int)keyCode});
    return btn;
  }

  wxButton* CreateCancelButton(wxSizer* sizer){
    wxButton* cancel = new wxButton(this,
      wxID_CANCEL, wxEmptyString, wxDefaultPosition,
      to_wx(big_button_size));
    sizer->Add(cancel);
    return cancel;
  }

  wxPoint GetRotateButtonPos() const{
    return m_rotateButton->GetPosition();
  }
  wxPoint GetCancelButtonPos() const{
    return m_cancelButton->GetPosition();
  }

  Key GetRotateAccelerator() const{
    return key::E;
  }
private:
  wxButton* m_cancelButton;
  wxButton* m_rotateButton;
};

class AngleChoicePanel : public wxPanel{
public:
  AngleChoicePanel(wxWindow* parent, const ArtContainer& art, const RotateChoicePanel& siblingPanel, cmd_event_func rotate)
    : wxPanel(parent, wxID_ANY),
      m_textCtrl(nullptr)
  {
    SetSize(siblingPanel.GetSize());
    Accelerators acc;
    CreateEntryRow(art, siblingPanel, acc, rotate);
    SetAcceleratorTable(accelerators(acc));
  }

  bool Modified() const{
    return m_textCtrl->GetValue() != "90";
  }

  Angle GetAngle() const{
    return Angle::Deg(parse_int_value(m_textCtrl, 90));
  }

  void CreateEntryRow(const ArtContainer& art, const RotateChoicePanel& siblingPanel, Accelerators& acc, cmd_event_func rotate){

    wxStaticText* label = new wxStaticText(this, wxID_ANY, "&Angle");

    m_textCtrl = new wxTextCtrl(this, wxID_ANY, "90");
    fit_size_to(m_textCtrl, "9999999");

    m_rotateBtn = new wxBitmapButton(this, wxID_ANY,
      art.Get(Icon::ROTATE_DIALOG_ROTATE));
    m_rotateBtn->SetInitialSize(to_wx(big_button_size));
    m_rotateBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, rotate);
    auto rotateKey = siblingPanel.GetRotateAccelerator();
    Bind(wxEVT_COMMAND_MENU_SELECTED, rotate, rotateKey);
    acc.push_back({rotateKey, (int)rotateKey});

    m_rotateBtn->SetPosition(siblingPanel.GetRotateButtonPos());

    wxButton* cancelButton = new wxButton(this, wxID_CANCEL,
      wxEmptyString, wxDefaultPosition, to_wx(big_button_size));
    cancelButton->SetPosition(siblingPanel.GetCancelButtonPos());

    m_textCtrl->SetPosition(to_the_left_middle_of(m_rotateBtn, m_textCtrl->GetSize()));
    label->SetPosition(to_the_left_middle_of(m_textCtrl, label->GetSize()));
  }

  void FocusEntry(){
    m_rotateBtn->SetFocus();
    m_textCtrl->SetFocus();
    m_textCtrl->SelectAll();
  }

  private:
  wxTextCtrl* m_textCtrl;
  wxButton* m_rotateBtn;
};

class RotateDialog : public wxDialog{
public:
  RotateDialog(wxWindow& parent, wxString targetName, const ArtContainer& art)
    : wxDialog(&parent, wxID_ANY, "Flip/Rotate " + targetName),
      m_cmdFunc(dummy_function),
      m_firstPanel(nullptr),
      m_secondPanel(nullptr),
      m_targetName(targetName)
  {
    m_firstPanel = new RotateChoicePanel(this, art,
      [=](wxCommandEvent&){SetFunctionAndClose(&context_flip_horizontal);},
      [=](wxCommandEvent&){SetFunctionAndClose(&context_flip_vertical);},
      [=](wxCommandEvent&){SwitchPanel();},
      [=](wxCommandEvent&){SelectLevelTool();});

    m_secondPanel = new AngleChoicePanel(this, art, *m_firstPanel,
      [=](wxCommandEvent&){RotateAndClose();});

    m_secondPanel->Hide();

    Fit();

    // Center on parent
    Centre(wxBOTH);
  }

  Command* GetCommand(const Canvas& canvas){
    return m_cmdFunc(canvas);
  }

private:

  void SetFunctionAndClose(cmd_func func){
    m_cmdFunc = func;
    EndModal(wxID_OK);
  }

  void RotateAndClose(){
    m_cmdFunc = [=](const Canvas& canvas){
      return m_secondPanel->Modified() ?
      context_rotate(canvas, m_secondPanel->GetAngle(), get_app_context().GetToolSettings().Get(ts_Bg)) :
      context_rotate90cw(canvas);
    };
    EndModal(wxID_OK);
  }

  void SelectLevelTool(){
    m_cmdFunc = nullptr;
    get_app_context().SelectTool(ToolId::LEVEL);
    EndModal(wxID_CANCEL);
  }

  void SwitchPanel(){
    m_firstPanel->Hide();
    m_firstPanel->Disable();
    m_secondPanel->Show();
    m_secondPanel->FocusEntry();
    SetTitle("Rotate " + m_targetName);
  }
  // The function that creates a Command. Depends on the user choice.
  cmd_func m_cmdFunc;
  RotateChoicePanel* m_firstPanel;
  AngleChoicePanel* m_secondPanel;
  wxString m_targetName;
};


Optional<Command*> show_rotate_dialog(wxWindow& parent, DialogFeedback&, const Canvas& canvas, const ArtContainer& art)
{
  RotateDialog dlg(parent, get_rotate_target_name(canvas), art);
  if (show_modal(dlg) == wxID_OK){
    return option(dlg.GetCommand(canvas));
  }
  return no_option();
}

dialog_func bind_show_rotate_dialog(const ArtContainer& art){
  return [&](wxWindow& window, DialogFeedback& feedback, const Canvas& canvas){
    return show_rotate_dialog(window, feedback, canvas, art);
  };
}

} // namespace
