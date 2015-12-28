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
#include "app/context-commands.hh"
#include "app/get-app-context.hh"
#include "app/canvas.hh"
#include "bitmap/bitmap.hh"
#include "bitmap/color.hh"
#include "commands/rescale-cmd.hh"
#include "commands/resize-cmd.hh"
#include "geo/geo-func.hh"
#include "geo/int-rect.hh"
#include "geo/size.hh"
#include "gui/art-container.hh"
#include "gui/layout.hh"
#include "gui/math-text-ctrl.hh"
#include "gui/resize-dialog.hh"
#include "gui/resize-dialog-options.hh"
#include "text/formatting.hh"
#include "util/apply-target.hh"
#include "util/color-bitmap-util.hh"
#include "util/command-util.hh"
#include "util/either.hh"
#include "util/object-util.hh"
#include "util-wx/clipboard.hh"
#include "util-wx/convert-wx.hh"
#include "util-wx/gui-util.hh"
#include "util-wx/key-codes.hh"

namespace faint{

class ButtonWithLabel{
  // Button with a label underneath (used for indicating which is the
  // default option, triggered with enter in the resize dialog)
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

static utf8_string serialize_size(const Size& size){
  std::stringstream ss;
  ss << size.w << "," << size.h;
  return to_faint(wxString(ss.str()));
}

static Optional<Size> deserialize_size(const utf8_string& str){
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

  return option(Size(width, height));
}

static Optional<Size> clipboard_get_size(){
  Clipboard clip;
  if (!clip.Good()){
    return no_option();
  }
  else if (auto maybeStr = clip.GetText()){
    return deserialize_size(maybeStr.Get());
  }

  return no_option();
}

static void clipboard_copy_size(const Size& size){
  Clipboard clip;
  if (clip.Good()){
    clip.SetText(serialize_size(size));
  }
}

static IntRect centered_resize_rect(const IntSize& newSize, const IntSize& oldSize){
  return IntRect(IntPoint(-(newSize.w - oldSize.w) / 2, -(newSize.h - oldSize.h) / 2),
    newSize);
}

// Helper for proportional update
static coord get_change_ratio(const MathTextCtrl& ctrl){
  return ctrl.GetValue() / ctrl.GetOldValue();
}

// Helper for proportional update
static void set_from_ratio(MathTextCtrl& ctrl, coord ratio){
  ctrl.SetValue(ctrl.GetOldValue() * ratio);
}

static wxBitmap create_resize_bitmap(const Paint& bg, const IntSize& size,
  const wxBitmap& image, const wxPoint& imagePos)
{
  IntSize patternSize(size / 2);

  // Note: clean_bitmap required for using wxMemoryDC::DrawBitmap on
  // MSW. Presumably due to wxWidgets issue #14403.
  wxBitmap bmp(clean_bitmap(to_wx_bmp(with_border(paint_bitmap(bg, size, patternSize)))));

  {
    wxMemoryDC dc(bmp);
    dc.DrawBitmap(image, imagePos);
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

static MathTextCtrl* add_entry_field(wxWindow* parent, wxBoxSizer* sizer, const wxString& labelStr, coord value){
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

template<typename SizeType>
class ResizeDialog : public wxDialog{
public:
  ResizeDialog(wxWindow& parent, const ArtContainer& icons, const wxString& title, const SizeType& size, const ResizeDialogOptions& options)
    : wxDialog(&parent, wxID_ANY, title),
      m_bmpSize(get_resize_icon_size(icons))
  {
    // Event and control identifiers
    static const int ID_TOP_LEFT = wxNewId();
    static const int ID_CENTER = wxNewId();
    static const int ID_SCALE = wxNewId();
    static const int ID_TOGGLE_PROPORTIONAL = wxNewId();
    static const int ID_TOGGLE_NEAREST = wxNewId();
    static const int ID_COPY_SIZE = wxNewId();
    static const int ID_PASTE_SIZE = wxNewId();

    // The outermost vertical sizer
    wxBoxSizer* rows = new wxBoxSizer(wxVERTICAL);

    // A proportion checkbox
    wxBoxSizer* optionsRow = new wxBoxSizer(wxHORIZONTAL);
    m_proportional = new wxCheckBox(this, wxID_ANY, "&Proportional");
    m_proportional->SetValue(options.proportional);
    optionsRow->Add(m_proportional, 1);

    if (options.bilinearOnly){
      m_nearest = nullptr;
    }
    else{
      m_nearest = new wxCheckBox(this, wxID_ANY, "&Nearest neighbour");
      m_nearest->SetValue(options.nearest);
      optionsRow->Add(m_nearest, 0, wxLEFT, item_spacing);
    }

    rows->Add(optionsRow, 0, wxLEFT|wxRIGHT|wxUP, panel_padding);
    rows->AddSpacer(item_spacing);

    // The row with the width and height entry fields
    wxBoxSizer* entryRow = new wxBoxSizer(wxHORIZONTAL);



    m_widthCtrl = add_entry_field(this, entryRow, "&Width", size.w);
    m_lastChanged = m_widthCtrl;
    entryRow->AddSpacer(item_spacing);
    m_heightCtrl = add_entry_field(this, entryRow, "&Height", size.h);
    rows->Add(entryRow, 0, wxLEFT|wxRIGHT, panel_padding);
    rows->AddSpacer(item_spacing);

    const wxBitmap& scaleBmp = icons.Get(Icon::RESIZEDLG_SCALE);

    // The buttons for resize type choice
    wxBoxSizer* buttonRow = new wxBoxSizer(wxHORIZONTAL);
    m_placementBmp = icons.Get(Icon::RESIZEDLG_PLACEMENT);
    if (!options.scaleOnly){
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
    m_resizeType = ResizeDialogOptions::UNKNOWN;
    if (options.scaleOnly){
      SetDefaultResizeOption(ResizeDialogOptions::RESCALE);
      // There's no reason to indicate the default when only one option
      // is available
      m_scale.HideLabel();
    }
    else {
      SetDefaultResizeOption(options.defaultButton);
    }

    SetAcceleratorTable(accelerators({
      // Since P, N are unused for text in the dialog, allow using
      // without Alt-modifier.
      {key::P, ID_TOGGLE_PROPORTIONAL},
      {key::N, ID_TOGGLE_NEAREST},

      // ..But also redundantly add Alt+<key> as accelerators, to prevent the
      // check-boxes stealing the focus from the entry fields
      {Alt+key::P, ID_TOGGLE_PROPORTIONAL},
      {Alt+key::N, ID_TOGGLE_NEAREST},

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

    Bind(EVT_MATH_TEXT_CONTROL_UPDATE,
      [&](const wxEvent& event){
        m_lastChanged = event.GetEventObject();
        UpdateProportions(true);
      });

    Bind(wxEVT_CHECKBOX,
      [&](const wxCommandEvent&){
        UpdateProportions(false);
      }, ID_TOGGLE_PROPORTIONAL);

    Bind(wxEVT_COMMAND_MENU_SELECTED,
      [&](const wxCommandEvent&){
        m_proportional->SetValue(!m_proportional->GetValue());
        UpdateProportions(false);
      }, ID_TOGGLE_PROPORTIONAL);

    if (!options.bilinearOnly){
      Bind(wxEVT_COMMAND_MENU_SELECTED,
        [&](wxCommandEvent&){
          m_nearest->SetValue(!m_nearest->GetValue());
        }, ID_TOGGLE_NEAREST);
    }

    Bind(wxEVT_COMMAND_MENU_SELECTED,
      [&](wxCommandEvent&){
        clipboard_copy_size({m_widthCtrl->GetValue(),
           m_heightCtrl->GetValue()});
      }, ID_COPY_SIZE);

    Bind(wxEVT_COMMAND_MENU_SELECTED,
      [&](const wxCommandEvent&){
        clipboard_get_size().Visit(
          [this](const Size& size){
            // Disable proportional resize, since both pasted values
            // should be kept as is.
            m_proportional->SetValue(false);
            m_widthCtrl->SetValue(size.w);
            m_heightCtrl->SetValue(size.h);
          });
       }
      , ID_PASTE_SIZE);

    Bind(wxEVT_BUTTON,
      [&](wxCommandEvent&){
        SetTypeAndClose(ResizeDialogOptions::RESIZE_TOP_LEFT,
          m_anchorTopLeft);
      }, ID_TOP_LEFT);

    Bind(wxEVT_BUTTON,
      [&](wxCommandEvent&){
        SetTypeAndClose(ResizeDialogOptions::RESIZE_CENTER,
          m_anchorCenter);
      },
      ID_CENTER);

    Bind(wxEVT_BUTTON,
      [&](wxCommandEvent&){
        SetTypeAndClose(ResizeDialogOptions::RESCALE, m_scale);
      }, ID_SCALE);
  }

  void SetBgColor(const Paint& bg){
    if (m_anchorTopLeft.Exists()){
      // Update the background color of the resize-buttons
      m_anchorTopLeft.button->SetBitmap(create_resize_bitmap(bg,
        m_bmpSize, m_placementBmp, wxPoint(0,0)));
      m_anchorCenter.button->SetBitmap(create_resize_bitmap(bg,
        m_bmpSize, m_placementBmp, wxPoint(16,16)));
    }
  }

  // GetSelection is valid only if ShowModal() returns wxID_OK
  std::pair<SizeType, ResizeDialogOptions> GetSelection() const{
    return {GetScaleSize(),
      ResizeDialogOptions(
      m_nearest != nullptr && m_nearest->GetValue(),
      m_proportional->GetValue(),
      false,
      false,
      m_resizeType)};
  }

private:
  SizeType GetScaleSize() const{
    return SizeType(
      (typename SizeType::value_type)m_widthCtrl->GetValue(),
      (typename SizeType::value_type)m_heightCtrl->GetValue());
  }

  bool GoodValues() const{
    return m_widthCtrl->GetValue() > 0 &&
      m_heightCtrl->GetValue() > 0;
  }

  bool ChangedValues() const{
    return m_widthCtrl->GetValue() != m_widthCtrl->GetOldValue() ||
      m_heightCtrl->GetValue() != m_heightCtrl->GetOldValue();
  }

  void SetDefaultResizeOption(ResizeDialogOptions::ResizeType type){
    m_scale.HideLabel();
    m_anchorCenter.HideLabel();
    m_anchorTopLeft.HideLabel();

    if (type == ResizeDialogOptions::RESCALE){
      SetDefaultItem(m_scale.button);
      m_scale.ShowLabel();
    }
    else if (type == ResizeDialogOptions::RESIZE_CENTER){
      SetDefaultItem(m_anchorCenter.button);
      m_anchorCenter.ShowLabel();
    }
    else {
      SetDefaultItem(m_anchorTopLeft.button);
      m_anchorTopLeft.ShowLabel();
    }
  }

  void SetTypeAndClose(ResizeDialogOptions::ResizeType resizeType, ButtonWithLabel& btn){
    btn.SetFocus();
    m_resizeType = resizeType;

    EndModal(GoodValues() && ChangedValues() ?
      wxID_OK : wxID_CANCEL);
  }

  void UpdateProportions(bool ignoreFocus){
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

  ResizeDialogOptions::ResizeType m_resizeType;
  MathTextCtrl* m_widthCtrl;
  MathTextCtrl* m_heightCtrl;
  wxObject* m_lastChanged;
  wxCheckBox* m_nearest;
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

  return dispatch_target(target,
    [&](OBJECT_SELECTION) -> Optional<Command*>{
      ResizeDialog<Size> dlg(parent,
        art,
        to_wx(space_sep("Resize",
            get_collective_type(canvas.GetObjectSelection()))),
        get_apply_target_size(canvas, target).Expect<Size>(),
        modified(app.GetDefaultResizeDialogOptions(),
          true /*scale only*/,
          true /*bilinear only*/));
      dlg.SetBgColor(app.Get(ts_Bg));

      if (dlg.ShowModal() != wxID_OK){
        return no_option();
      }
      auto result = dlg.GetSelection();
      Size size = result.first;
      if (area(size) <= 0){
        return no_option();
      }
      const ResizeDialogOptions& opts = result.second;
      app.SetDefaultResizeDialogOptions(opts);
      return option(context_scale_objects(canvas, size));
    },

    [&](RASTER_SELECTION) -> Optional<Command*>{
      ResizeDialog<IntSize> dlg(parent,
        art,
        "Scale Selection",
        get_apply_target_size(canvas, target).Expect<IntSize>(),
        modified(app.GetDefaultResizeDialogOptions(),
          true /*scale only*/,
          false /*bilinear only*/));
      dlg.SetBgColor(app.Get(ts_Bg));

      if (dlg.ShowModal() != wxID_OK){
        return no_option();
      }
      auto result = dlg.GetSelection();
      IntSize size = result.first;
      if (area(size) <= 0){
        return no_option();
      }
      const ResizeDialogOptions& opts = result.second;
      app.SetDefaultResizeDialogOptions(opts);
      return option(get_scale_raster_selection_command(canvas.GetImage(),
          size,
        opts.nearest ? ScaleQuality::NEAREST : ScaleQuality::BILINEAR));
    },

    [&](IMAGE) -> Optional<Command*>{
      ResizeDialog<IntSize> dlg(parent,
        art,
        "Resize or Scale Image",
        get_apply_target_size(canvas, target).Expect<IntSize>(),
        modified(app.GetDefaultResizeDialogOptions(),
          false /*scale only*/,
          false /*bilinear only*/));
      dlg.SetBgColor(app.Get(ts_Bg));
      if (dlg.ShowModal() != wxID_OK){
        return no_option();
      }
      auto result = dlg.GetSelection();
      IntSize size = result.first;
      if (area(size) <= 0){
        return no_option();
      }
      const ResizeDialogOptions& opts = result.second;
      app.SetDefaultResizeDialogOptions(opts);
      if (opts.defaultButton == ResizeDialogOptions::RESIZE_TOP_LEFT){
        return option(get_resize_command(canvas.GetBackground().Get<Bitmap>(),
          rect_from_size(size), get_app_context().Get(ts_Bg)));
      }
      else if (opts.defaultButton == ResizeDialogOptions::RESIZE_CENTER){
        return option(get_resize_command(canvas.GetBitmap(),
            centered_resize_rect(size, canvas.GetSize()),
            get_app_context().Get(ts_Bg)));
      }
      else if (opts.defaultButton == ResizeDialogOptions::RESCALE){
        return option(rescale_command(size, opts.nearest?
            ScaleQuality::NEAREST : ScaleQuality::BILINEAR));
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
