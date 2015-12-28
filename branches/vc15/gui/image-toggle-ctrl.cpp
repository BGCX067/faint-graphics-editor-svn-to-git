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

#include "wx/dcclient.h"
#include "gui/bitmap-list-ctrl.hh"
#include "gui/image-toggle-ctrl.hh"
#include "text/formatting.hh"
#include "util/setting-id.hh"
#include "util/status-interface.hh"
#include "util-wx/bind-event.hh"
#include "util-wx/convert-wx.hh"
#include "util-wx/gui-util.hh"

namespace faint{

class ImageToggleCtrl : public IntSettingCtrl{
  // Adds mapping of the image indexes in a BitmapListCtrl to values
  // for an IntSetting.
public:
  ImageToggleCtrl(wxWindow* parent, const IntSetting& setting,
    const IntSize& imageSize,
    StatusInterface& status,
    Axis dir,
    const IntSize& spacing)
    : IntSettingCtrl(parent, setting)
  {
    m_bitmapList = new BitmapListCtrl(this,
      imageSize,
      status,
      dir,
      spacing);
    SetWindowStyleFlag(wxBORDER_SIMPLE);

    Bind(EVT_FAINT_BITMAPLIST_SELECTION,
      [this](wxEvent&){
        SendChangeEvent();
      });
  }

  void AddButton(const wxBitmap& bmp, int value, const utf8_string& statusText){
    index_t index = m_bitmapList->Add(bmp, statusText);
    m_indexToValue[index] = value;
    m_valueToIndex[value] = index;
    SetClientSize(m_bitmapList->GetSize());
    SetMinSize(m_bitmapList->GetSize());
  }

  int GetValue() const override{
    auto index = m_bitmapList->GetSelection();
    auto it = m_indexToValue.find(index);
    assert(it != m_indexToValue.end());
    return it->second;
  }

  void SetValue(int value) override{
    auto index = m_valueToIndex[value];
    m_bitmapList->SetSelection(index);
  }

protected:
  void DoSetToolTip(wxToolTip* toolTip) override{
    m_bitmapList->SetToolTip(toolTip);
  }

private:
  BitmapListCtrl* m_bitmapList;
  std::map<index_t, int> m_indexToValue;
  std::map<int, index_t> m_valueToIndex;
};

IntSettingCtrl* new_image_toggle(wxWindow* parent,
  const IntSetting& setting,
  const IntSize& imageSize,
  StatusInterface& status,
  const tooltip_t& toolTip,
  const ToggleImage& image1,
  const ToggleImage& image2,
  Axis dir,
  IntSize spacing)
{
  ImageToggleCtrl* ctrl = new ImageToggleCtrl(parent,
    setting,
    imageSize,
    status,
    dir,
    spacing);
  ctrl->SetToolTip(to_wx(toolTip.Get()));
  ctrl->AddButton(image1.bmp, image1.value, image1.statusText);
  ctrl->AddButton(image2.bmp, image2.value, image2.statusText);
  return ctrl;
}

IntSettingCtrl* new_image_toggle(wxWindow* parent,
  const IntSetting& setting,
  const IntSize& bitmapSize,
  StatusInterface& status,
  const tooltip_t& toolTip,
  const ToggleImage& image1,
  const ToggleImage& image2,
  const ToggleImage& image3,
  Axis dir,
  IntSize spacing)
{
  ImageToggleCtrl* ctrl = new ImageToggleCtrl(parent,
    setting,
    bitmapSize,
    status,
    dir,
    spacing);
  ctrl->SetToolTip(to_wx(toolTip.Get()));
  ctrl->AddButton(image1.bmp, image1.value, image1.statusText);
  ctrl->AddButton(image2.bmp, image2.value, image2.statusText);
  ctrl->AddButton(image3.bmp, image3.value, image3.statusText);
  return ctrl;
}

void update_status(StatusInterface& status, const BoolSetting& s, bool value){
  status.SetMainText(space_sep(value ? "Disable" : "Enable",
      setting_name(untyped(s))));
}

class BoolImageToggle : public BoolSettingControl {
public:
  BoolImageToggle(wxWindow* parent,
    const BoolSetting& s,
    const wxBitmap& bmp,
    StatusInterface& status)
    : BoolSettingControl(parent, s),
      m_bitmap(bmp),
      m_value(false),
      m_status(status)
  {
    SetInitialSize(wxSize(50,28));
    SetWindowStyleFlag(wxBORDER_SIMPLE);

    bind(wxEVT_LEFT_DOWN, wxEVT_LEFT_DCLICK, this,
      [this](wxMouseEvent&){
        m_value = !m_value;
        update_status(m_status, GetSetting(), m_value);
        SendChangeEvent();
        Refresh();
      });

    Bind(wxEVT_LEAVE_WINDOW,
      [this](wxMouseEvent& event){
        m_status.SetMainText("");
        event.Skip();
      });

    Bind(wxEVT_MOTION,
      [this](wxMouseEvent& event){
        update_status(m_status, GetSetting(), m_value);
        event.Skip();
      });

    Bind(wxEVT_PAINT,
      [this](wxPaintEvent&){
        wxPaintDC dc(this);
        wxColour deselectedColor(get_gui_deselected_color());
        wxColour selectedColor(get_gui_selected_color());
        if (m_value){
          dc.SetPen(wxPen(selectedColor));
          dc.SetBrush(wxBrush(selectedColor));
        }
        else {
          dc.SetPen(wxPen(deselectedColor));
          dc.SetBrush(wxBrush(deselectedColor));
        }
        dc.DrawRectangle(0,0, 50, 28);
        dc.DrawBitmap(m_bitmap, 12, 2);
      });
  }

  bool GetValue() const override{
    return m_value;
  }

  void SetValue(bool value) override{
    m_value = value;
  }

private:
  wxBitmap m_bitmap;
  bool m_value;
  StatusInterface& m_status;
};

BoolSettingControl* new_bool_image_toggle(wxWindow* parent,
  const BoolSetting& setting,
  const wxBitmap& bmp,
  StatusInterface& status,
  const tooltip_t& toolTip)
{
  BoolImageToggle* ctrl = new BoolImageToggle(parent,
    setting,
    bmp,
    status);
  ctrl->SetToolTip(to_wx(toolTip.Get()));
  return ctrl;
}

} // namespace
