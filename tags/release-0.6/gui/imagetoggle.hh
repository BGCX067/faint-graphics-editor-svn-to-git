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

#ifndef FAINT_IMAGETOGGLE_HH
#define FAINT_IMAGETOGGLE_HH
#include <map>
#include "toolsettingctrl.hh"
#include "wx/wx.h" // Fixme

class BitmapListCtrl;
class wxBitmap;
class StatusInterface;

class ImageToggleCtrl : public IntSettingCtrl{
  // Adds mapping of the image indexes in a BitmapListCtrl to values
  // for an IntSetting.
public:
  ImageToggleCtrl( wxWindow* parent, const IntSetting&, const wxSize&, StatusInterface&, wxOrientation dir=wxVERTICAL, unsigned int hSpace=10, unsigned int vSpace=5 );
  void AddButton( const wxBitmap&, int value, const std::string& status );
  int GetValue() const;
  void SetValue(int);
protected:
  void DoSetToolTip(wxToolTip*);
private:
  void OnSelection(wxCommandEvent&);
  BitmapListCtrl* m_bitmapList;
  std::map<int, int> m_indexToValue;
  std::map<int, int> m_valueToIndex;
  IntSetting m_setting; // Fixme  
  DECLARE_EVENT_TABLE()
};

class BoolImageToggle : public BoolSettingControl {
public:
  BoolImageToggle( wxWindow* parent, const BoolSetting&, const wxBitmap&, StatusInterface& );
  bool GetValue() const;
  void SetValue(bool);
private:
  void OnLeaveWindow(wxMouseEvent&);
  void OnLeftDown(wxMouseEvent&);
  void OnMotion(wxMouseEvent&);
  void OnPaint(wxPaintEvent&);
  wxBitmap m_bitmap;
  bool m_value;
  StatusInterface& m_status;
  DECLARE_EVENT_TABLE()
};

#endif
