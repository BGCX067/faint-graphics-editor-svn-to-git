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

class BitmapListCtrl;

class ImageToggleCtrl : public IntSettingCtrl{
public:
  ImageToggleCtrl( wxWindow* parent, const IntSetting& setting, wxSize bitmapSize, wxOrientation dir = wxVERTICAL, unsigned int hSpace=10, unsigned int vSpace=5 );
  void AddButton( wxBitmap* bmp, int value_);
  virtual int GetValue() const;
  virtual void SetValue( int);      
private:
  void OnSelection( wxCommandEvent& event );
  BitmapListCtrl* bitmapList;
  std::map<int, int> indexToValue;
  std::map<int, int> valueToIndex;
  IntSetting m_setting;
  DECLARE_EVENT_TABLE()  
};

#endif
