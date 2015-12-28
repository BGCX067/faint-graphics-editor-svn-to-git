// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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
#include "wx/bmpbuttn.h"
#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/dcmemory.h"
#include "wx/dialog.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"
#include "app/get-app-context.hh"
#include "bitmap/bitmap.hh"
#include "commands/rescale-cmd.hh"
#include "commands/resize-cmd.hh"
#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "geo/size.hh"
#include "gui/layout.hh"
#include "gui/math-text-ctrl.hh"
#include "gui/resize-dialog.hh"
#include "gui/resize-dialog-settings.hh"
#include "text/formatting.hh"
#include "util/apply-target.hh"
#include "util/art-container.hh"
#include "util/canvas.hh"
#include "util/clipboard.hh"
#include "util/color.hh"
#include "util/color-bitmap-util.hh"
#include "util/command-util.hh"
#include "util/context-commands.hh"
#include "util/convert-wx.hh"
#include "util/either.hh"
#include "util/gui-util.hh"
#include "util/object-util.hh"

namespace faint{

class ButtonWithLabel{
public:
  ButtonWithLabel()
    : label(nullptr),
      button(nullptr)
  {}

  bool Exists() const{
    return this->button != nullptr;
  }

  void HideLabel(){
    if (this->label != nullptr){
      this->label->Hide();
    }
  }

  void SetFocus(){
    if (this->button != nullptr){
      this->button->SetFocus();
    }
  }

  void ShowLabel(){
    if (this->label != nullptr){
      this->label->Show();
    }
  }
  wxStaticText* label;
  wxBitmapButton* button;
};

static IntRect centered_resize_rect(const IntSize& newSize, const IntSize& oldSize){
  return IntRect(IntPoint(-(newSize.w - oldSize.w) / 2, -(newSize.h - oldSize.h) / 2),
    newSize);
}

static wxString get_resize_dialog_title(const Canvas& canvas, ApplyTarget target){
  return dispatch_target(target,
    [&](OBJECT_SELECTION){
      return to_wx(space_sep("Resize",
          get_collective_name(canvas.GetObjectSelection())));
    },

    [](RASTER_SELECTION){
      return wxString("Scale Selection");
    },

    [](IMAGE){
      return wxString("Resize or Scale Image");
    });
}

static bool get_scale_only(ApplyTarget target){
  return dispatch_target(target,
    [](OBJECT_SELECTION){return true;},
    [](RASTER_SELECTION){return true;},
    [](IMAGE){return false;});
}

// Helper for proportional update
static coord get_change_ratio(const MathTextCtrl& ctrl){
  return floated(ctrl.GetValue()) / floated(ctrl.GetOldValue());
}

// Helper for proportional update
static void set_from_ratio(MathTextCtrl& ctrl, coord ratio){
  ctrl.SetValue(rounded(floated(ctrl.GetOldValue()) * ratio));
}

static utf8_string serialize_size(const IntSize& size){
  std::stringstream ss;
  ss << size.w << "," << size.h;
  return to_faint(wxString(ss.str()));
}

static Optional<IntSize> deserialize_size(const utf8_string& str){
  wxString wxStr(to_wx(str));
  wxArrayString strs = wxSplit(to_wx(str), ',', '\0');
  if (strs.GetCount() != 2){
    return no_option();
  }

  long width;
  if (!strs[0].ToLong(&width)){
    return no_option();
  }
  if (width <= 0){
    return no_option();
  }

  long height;
  if (!strs[1].ToLong(&height)){
    return no_option();
  }
  if (height <= 0){
    return no_option();
  }

  return option(IntSize((int)width, (int)height));
}

static wxBitmap create_resize_bitmap(const Paint& src, const IntSize& size, const wxBitmap& image, const wxPoint& imagePos){
  IntSize patternSize(size / 2);
  // Note: clean_bitmap required for using wxMemoryDC::DrawBitmap on
  // MSW. Presumably due to wxWidgets issue #14403.
  wxBitmap bmp(clean_bitmap(to_wx_bmp(with_border(paint_bitmap(src, size, patternSize)))));
  {
    wxMemoryDC dc(bmp);
    dc.DrawBitmap(image,imagePos);
  }
  return bmp;
}

static IntSize get_resize_icon_size(const ArtContainer& icons){
  return to_faint(icons.Get(Icon::RESIZEDLG_SCALE).GetSize());
}

static MathTextCtrl* add_entry_field(wxWindow* parent, wxBoxSizer* sizer, const wxString& labelStr, int value){
  wxStaticText* label = new wxStaticText(parent, wxID_ANY, labelStr);
  sizer->Add(label, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 10);
  MathTextCtrl* ctrl = new MathTextCtrl(parent, value);
  ctrl->FitSizeTo("1024 (100%)");
  sizer->Add(ctrl, 0, wxALIGN_CENTER_VERTICAL);
  return ctrl;
}

static ButtonWithLabel add_button(wxWindow* parent, wxBoxSizer* sizer, int id, wxString toolTip, const wxBitmap& bmp){
  // Adds a button with the text "(Default)" as a static text label
  // below the button, for indicating which button the Enter-key
  // targets. The label should only be shown for the current default
  // button.

  ButtonWithLabel buttonAndLabel;
  buttonAndLabel.button = new wxBitmapButton(parent, id, bmp,
    wxDefaultPosition, wxDefaultSize);
  buttonAndLabel.button->SetToolTip(toolTip);
  buttonAndLabel.label = new wxStaticText(parent, wxID_ANY, "(Default)");
  buttonAndLabel.button->SetInitialSize(to_wx(big_button_size));

  wxBoxSizer* oneButtonSizer = new wxBoxSizer(wxVERTICAL);
  oneButtonSizer->Add(buttonAndLabel.button);
  oneButtonSizer->Add(buttonAndLabel.label, 0, wxALIGN_CENTER);
  sizer->Add(oneButtonSizer);
  return buttonAndLabel;
}

class ResizeDialog : public wxDialog{
public:
  ResizeDialog(wxWindow& parent, const ArtContainer& icons, const wxString& title, const ResizeDialogSettings& settings)
    : wxDialog(&parent, wxID_ANY, title),
      m_bmpSize(get_resize_icon_size(icons))
  {
    // Event and control identifiers
    static const int ID_TOP_LEFT = wxNewId();
    static const int ID_CENTER = wxNewId();
    static const int ID_SCALE = wxNewId();
    static const int ID_TOGGLE_CHECK = wxNewId();
    static const int ID_COPY_SIZE = wxNewId();
    static const int ID_PASTE_SIZE = wxNewId();

    // The outermost vertical sizer
    wxBoxSizer* rows = new wxBoxSizer(wxVERTICAL);

    // A proportion checkbox
    wxBoxSizer* proportionRow = new wxBoxSizer(wxHORIZONTAL);
    m_proportional = new wxCheckBox(this, wxID_ANY, "&Proportional");
    m_proportional->SetValue(settings.proportional);
    proportionRow->Add(m_proportional, 0);
    rows->Add(proportionRow, 0, wxLEFT|wxRIGHT|wxUP, panel_padding);
    rows->AddSpacer(item_spacing);

    // The row with the width and height entry fields
    wxBoxSizer* entryRow = new wxBoxSizer(wxHORIZONTAL);
    m_widthCtrl = add_entry_field(this, entryRow, "&Width", settings.size.w);
    entryRow->AddSpacer(item_spacing);
    m_lastChanged = m_widthCtrl;
    m_heightCtrl = add_entry_field(this, entryRow, "&Height", settings.size.h);
    rows->Add(entryRow, 0, wxLEFT|wxRIGHT, panel_padding);
    rows->AddSpacer(item_spacing);

    const wxBitmap& scaleBmp = icons.Get(Icon::RESIZEDLG_SCALE);

    // The buttons for resize type choice
    wxBoxSizer* buttonRow = new wxBoxSizer(wxHORIZONTAL);
    m_placementBmp = icons.Get(Icon::RESIZEDLG_PLACEMENT);
    if (!settings.scaleOnly){
      m_anchorTopLeft = add_button(this, buttonRow, ID_TOP_LEFT,
        "Resize drawing area right and down (Key: Q)",
        to_wx_bmp(color_bitmap(Color(255,255,255), m_bmpSize)));
      m_anchorCenter = add_button(this, buttonRow, ID_CENTER,
        "Resize drawing area in all directions (Key: W)",
        to_wx_bmp(color_bitmap(Color(255,255,255), m_bmpSize)));
    }
    m_scale = add_button(this, buttonRow, ID_SCALE,
      "Scale the image (Key: E)",
      scaleBmp);

    m_cancel = new wxButton(this, wxID_CANCEL, wxEmptyString,
      wxDefaultPosition, to_wx(big_button_size));
    buttonRow->Add(m_cancel, 0);
    rows->Add(buttonRow, 0, wxLEFT|wxRIGHT|wxDOWN, panel_padding);

    // Initialize size and layout
    SetSizerAndFit(rows);

    m_widthCtrl->SetFocus();
    m_resizeType = ResizeDialogSettings::UNKNOWN;
    if (settings.scaleOnly){
      SetDefaultResizeOption(ResizeDialogSettings::RESCALE);
      // There's no reason to indicate the default when only one option
      // is available
      m_scale.HideLabel();
    }
    else {
      SetDefaultResizeOption(settings.defaultButton);
    }

    SetAcceleratorTable(accelerators({
      // Redundantly add Alt+P as an accelerator, to prevent the check-box
      // from stealing the focus from the entry fields
      {Alt+key::P, ID_TOGGLE_CHECK},

      // Since P is unused in the dialog, allow using P without modifier
      // to toggle the check-box
      {key::P, ID_TOGGLE_CHECK},

      // Each resize option
      {key::Q, ID_TOP_LEFT},
      {key::W, ID_CENTER},
      {key::E, ID_SCALE},

      // Alt+C copies both the width and height
     {Alt+key::C, ID_COPY_SIZE},
     {Alt+key::V, ID_PASTE_SIZE}
    }));

    // Center on parent
    Centre(wxBOTH);

    typedef ResizeDialog Me;
    Bind(EVT_MATH_TEXT_CONTROL_UPDATE, &Me::OnValueUpdate, this);
    Bind(wxEVT_CHECKBOX, &Me::OnToggleProportional, this);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &Me::OnToggleProportionalKB, this, ID_TOGGLE_CHECK);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &Me::OnCopySize, this, ID_COPY_SIZE);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &Me::OnPasteSize, this, ID_PASTE_SIZE);
    Bind(wxEVT_BUTTON, &Me::OnTopLeftResize, this, ID_TOP_LEFT);
    Bind(wxEVT_BUTTON, &Me::OnCenterResize, this, ID_CENTER);
    Bind(wxEVT_BUTTON, &Me::OnScale, this, ID_SCALE);
  }

  void SetBgColor(const Paint& src){
    if (m_anchorTopLeft.Exists()){
      // Update the background color of the resize-buttons
      m_anchorTopLeft.button->SetBitmap(create_resize_bitmap(src, m_bmpSize, m_placementBmp, wxPoint(0,0)));
      m_anchorCenter.button->SetBitmap(create_resize_bitmap(src, m_bmpSize, m_placementBmp, wxPoint(16,16)));
    }
  }

  // GetSelection is valid only if ShowModal() returns wxID_OK
  ResizeDialogSettings GetSelection() const{
    return ResizeDialogSettings(IntSize(m_widthCtrl->GetValue(), m_heightCtrl->GetValue()), m_proportional->GetValue(), false, m_resizeType);
  }

private:
  bool GoodValues() const{
    return m_widthCtrl->GetValue() > 0 &&
      m_heightCtrl->GetValue() > 0;
  }

  bool ChangedValues() const{
    return m_widthCtrl->GetValue() != m_widthCtrl->GetOldValue() ||
      m_heightCtrl->GetValue() != m_heightCtrl->GetOldValue();
  }

  void SetDefaultResizeOption(ResizeDialogSettings::ResizeType type){
    m_scale.HideLabel();
    m_anchorCenter.HideLabel();
    m_anchorTopLeft.HideLabel();

    if (type == ResizeDialogSettings::RESCALE){
      SetDefaultItem(m_scale.button);
      m_scale.ShowLabel();
    }
    else if (type == ResizeDialogSettings::RESIZE_CENTER){
      SetDefaultItem(m_anchorCenter.button);
      m_anchorCenter.ShowLabel();
    }
    else {
      SetDefaultItem(m_anchorTopLeft.button);
      m_anchorTopLeft.ShowLabel();
    }
  }

  void SetTypeAndClose(ResizeDialogSettings::ResizeType resizeType, ButtonWithLabel& btn){
    btn.SetFocus();
    m_resizeType = resizeType;

    EndModal(GoodValues() && ChangedValues() ?
      wxID_OK : wxID_CANCEL);
  }

  void UpdateProportions(bool ignoreFocus){
    // Todo: This could be improved further. Currently, the focused
    // control is never modified like this.
    // - The currently edited field should be updatable if it has not changed
    // after getting focus.
    // Example: edit w, tab to h. Toggle proportional - h should change, not w.
    // However, the edit position etc. should be set properly in h after adjusting, and
    // it should show the editable values (not with percentage etc)
    if (m_proportional->GetValue()){
      if ((!ignoreFocus && m_widthCtrl->HasFocus()) ||
        ((ignoreFocus || !m_heightCtrl->HasFocus()) && m_lastChanged == m_widthCtrl)){
        set_from_ratio(*m_heightCtrl, get_change_ratio(*m_widthCtrl));
      }

      else {
        set_from_ratio(*m_widthCtrl, get_change_ratio(*m_heightCtrl));
      }
    }
  }

  void OnCopySize(wxCommandEvent&){
    Clipboard clip;
    if (clip.Good()){
      IntSize size(m_widthCtrl->GetValue(), m_heightCtrl->GetValue());
      clip.SetText(serialize_size(size));
    }
  }

  void OnPasteSize(wxCommandEvent&){
    Clipboard clip;
    if (!clip.Good()){
      return;
    }
    if (auto maybeStr = clip.GetText()){
      if (auto maybeSize = deserialize_size(maybeStr.Get())){
        // Disable proportional resize, since both pasted values
        // should be kept as is.
        m_proportional->SetValue(false);
        const IntSize& size(maybeSize.Get());
        m_widthCtrl->SetValue(size.w);
        m_heightCtrl->SetValue(size.h);
      }
    }
  }

  void OnValueUpdate(wxEvent& event){
    m_lastChanged = event.GetEventObject();
    UpdateProportions(true);
  }

  void OnToggleProportional(wxCommandEvent&){
    UpdateProportions(false);
  }

  void OnToggleProportionalKB(wxCommandEvent&){
    m_proportional->SetValue(!m_proportional->GetValue());
    UpdateProportions(false);
  }

  void OnTopLeftResize(wxCommandEvent&){
    SetTypeAndClose(ResizeDialogSettings::RESIZE_TOP_LEFT,
      m_anchorTopLeft);
  }

  void OnCenterResize(wxCommandEvent&){
    SetTypeAndClose(ResizeDialogSettings::RESIZE_CENTER,
      m_anchorCenter);
  }

  void OnScale(wxCommandEvent&){
    SetTypeAndClose(ResizeDialogSettings::RESCALE,
      m_scale);
  }

  ResizeDialogSettings::ResizeType m_resizeType;
  MathTextCtrl* m_widthCtrl;
  MathTextCtrl* m_heightCtrl;
  wxObject* m_lastChanged;
  wxCheckBox* m_proportional;
  ButtonWithLabel m_anchorTopLeft;
  ButtonWithLabel m_anchorCenter;
  ButtonWithLabel m_scale;
  wxButton* m_cancel;
  wxBitmap m_placementBmp;
  IntSize m_bmpSize;
};

Optional<Command*> show_resize_dialog(wxWindow& parent, DialogFeedback&, const Canvas& canvas, const ArtContainer& art)
{
  ApplyTarget target(get_apply_target(canvas));
  AppContext& app = get_app_context();
  ResizeDialog dlg(parent,
    art,
    get_resize_dialog_title(canvas, target),
    modified(app.GetDefaultResizeDialogSettings(),
      floored(get_apply_target_size(canvas, target)), // Fixme: Floor bad for objects
      get_scale_only(target)));
  dlg.SetBgColor(app.Get(ts_Bg));
  if (dlg.ShowModal() != wxID_OK){
    return no_option();
  }

  ResizeDialogSettings s = dlg.GetSelection();
  if (area(s.size) <= 0){
    // Invalid size specified, do nothing.
    return no_option();
  }

  app.SetDefaultResizeDialogSettings(s);

  return dispatch_target(target,
    [&](OBJECT_SELECTION){
      // Fixme: Should already be floated.
      return option(context_scale_objects(canvas, floated(s.size)));
    },

    [&](RASTER_SELECTION){
      return option(get_scale_raster_selection_command(canvas.GetImage(), s.size));
    },

    [&](IMAGE){
      // Scale or resize the entire image
      if (s.defaultButton == ResizeDialogSettings::RESIZE_TOP_LEFT){

        return option(get_resize_command(canvas.GetBackground().Get<Bitmap>(),
            rect_from_size(s.size), get_app_context().Get(ts_Bg)));
      }
      else if (s.defaultButton == ResizeDialogSettings::RESIZE_CENTER){
        return option(get_resize_command(canvas.GetBitmap(),
            centered_resize_rect(s.size, canvas.GetSize()),
            get_app_context().Get(ts_Bg)));
      }
      else if (s.defaultButton == ResizeDialogSettings::RESCALE){
          return option(rescale_command(s.size, ScaleQuality::BILINEAR));
      }
      else{
        assert(false);
        return Optional<Command*>();
      }
    });
}

dialog_func bind_show_resize_dialog(const ArtContainer& art){
  return [&](wxWindow& window, DialogFeedback& feedback, const Canvas& canvas){
    return show_resize_dialog(window, feedback, canvas, art);
  };
}

} // namespace
