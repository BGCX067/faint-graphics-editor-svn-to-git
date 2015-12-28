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
#include "wx/dialog.h"
#include "wx/sizer.h"
#include "app/app.hh" // For retrieving artcontainer
#include "gui/layout.hh"
#include "gui/rotate-dialog.hh"
#include "util/commandutil.hh"
#include "util/context-commands.hh"
#include "util/convertwx.hh"
#include "util/formatting.hh"
#include "util/guiutil.hh"
#include "util/objutil.hh"

using faint::Accelerators;

static Command* dummy_function( CanvasInterface& ){
  // Avoids leaving RotateDialogImpl::m_cmdFunc uninitialized
  return nullptr;
}

typedef std::function<decltype(dummy_function)> cmd_func;

static wxString get_rotate_dialog_title(CanvasInterface& canvas){
  ApplyTarget target(get_apply_target(canvas));
  if ( target == APPLY_OBJECT_SELECTION ){
    return space_sep("Flip/Rotate",
      get_collective_name(canvas.GetObjectSelection()));
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    return "Flip/Rotate Selection";
  }
  assert( target == APPLY_IMAGE );
  return "Flip/Rotate Image";
}

class RotateDialogImpl : public wxDialog{
public:
  RotateDialogImpl( wxWindow* parent, const wxString& title )
    : wxDialog( parent, wxID_ANY, title),
      m_cmdFunc(dummy_function)
  {
    Accelerators accelerators;
    const ArtContainer& art = wxGetApp().GetArtContainer();
    wxBoxSizer* rows = new wxBoxSizer( wxVERTICAL );

    CreateButtons(art, rows, accelerators);
    SetSizerAndFit(rows);

    SetAcceleratorTable( accelerators.Get() );

    // Center on parent
    Centre( wxBOTH );
  }

  Command* GetCommand(CanvasInterface& canvas){
    return m_cmdFunc(canvas);
  }

private:
  void CreateButtons(const ArtContainer& art, wxSizer* rows, Accelerators& acc ){
    wxBoxSizer* buttonRow = new wxBoxSizer(wxHORIZONTAL);

    CreateButton(&context_flip_horizontal,
      "Flip Horizontally",
      art.Get(Icon::ROTATE_DIALOG_FLIP_HORIZONTAL),
      acc, (int)'Q',
      buttonRow);

    CreateButton(&context_flip_vertical,
      "Flip Vertically",
      art.Get(Icon::ROTATE_DIALOG_FLIP_VERTICAL),
      acc, (int)'W',
      buttonRow);

    CreateButton(&context_rotate90cw,
      "Rotate 90 Degrees Clockwise",
      art.Get(Icon::ROTATE_DIALOG_ROTATE90_CW),
      acc, (int)'E',
      buttonRow);

    CreateCancelButton(buttonRow);

    rows->Add( buttonRow, 0, wxALL, faint::panel_padding );
  }

  void CreateButton(cmd_func func, const wxString& toolTip, const wxBitmap& bmp, Accelerators& acc, int keyCode, wxSizer* sizer ){
    // Create and add the button
    wxButton* btn = new wxBitmapButton(this, wxID_ANY, bmp);
    btn->SetInitialSize(to_wx(faint::big_button_size));
    btn->SetToolTip(toolTip);
    sizer->Add(btn);

    // Bind click and keyboard accelerator events
    auto handler = [=](wxCommandEvent&){
      SetFunctionAndClose(func);
    };
    btn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, handler);
    Bind(wxEVT_COMMAND_MENU_SELECTED, handler, keyCode);
    acc.Add(wxACCEL_NORMAL, keyCode, keyCode);
  }

  void CreateCancelButton(wxSizer* sizer){
    wxButton* cancel = new wxButton(this,
      wxID_CANCEL, wxEmptyString, wxDefaultPosition,
      to_wx(faint::big_button_size));
    sizer->Add(cancel);
  }

  void SetFunctionAndClose(cmd_func func){
    m_cmdFunc = func;
    EndModal(wxID_OK);
  }

  // The function that creates a Command. Depends on the user choice.
  cmd_func m_cmdFunc;
};

RotateDialog::RotateDialog(){}

Command* RotateDialog::GetCommand(){
  return m_command.Retrieve();
}

bool RotateDialog::ShowModal(wxWindow* parent, DialogFeedback& feedback){
  CanvasInterface& canvas(feedback.GetCanvas());
  RotateDialogImpl dlg( parent, get_rotate_dialog_title(canvas));
  if ( faint::show_modal(dlg) == wxID_OK ){
    m_command.Set(dlg.GetCommand(canvas));
    return true;
  }
  return false;
}
