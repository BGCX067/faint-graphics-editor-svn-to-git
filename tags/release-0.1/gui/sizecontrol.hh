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

#ifndef FAINT_SIZECONTROL_HH
#define FAINT_SIZECONTROL_HH
#include "wx/spinctrl.h"
#include "toolsettingctrl.hh"
#include "settings.hh"

class IntSizeControl : public IntSettingCtrl{
public:
  IntSizeControl( wxWindow* parent, wxSize size, const IntSetting& setting, int value, const std::string& label="" );
  virtual int GetValue() const;
  virtual void SetValue( int );
private:
  void OnSpin(wxSpinEvent&);
  void OnEnter(wxCommandEvent&);
  wxSpinCtrl* m_spinCtrl;
  DECLARE_EVENT_TABLE()
};

class FloatSizeControl : public FloatSettingControl{
public:
  FloatSizeControl( wxWindow* parent, wxSize size, const FloatSetting& setting, faint::coord value, const std::string& label="");
  virtual faint::coord GetValue() const;
  virtual void SetValue( faint::coord );
private:
  void OnSpin(wxSpinDoubleEvent&);
  void OnEnter(wxCommandEvent&);
  wxSpinCtrlDouble* m_spinCtrl;
  DECLARE_EVENT_TABLE()
};

class SemiFloatSizeControl : public FloatSettingControl{
public:
  SemiFloatSizeControl( wxWindow* parent, wxSize size, const FloatSetting& setting, faint::coord value, const std::string& label="" );
  virtual faint::coord GetValue() const;
  virtual void SetValue( faint::coord );
private:
  void OnSpin(wxSpinEvent&);
  void OnEnter(wxCommandEvent&);
  wxSpinCtrl* m_spinCtrl;
  DECLARE_EVENT_TABLE()
};

#endif
