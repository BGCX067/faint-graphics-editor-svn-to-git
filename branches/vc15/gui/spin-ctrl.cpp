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

#include "wx/textctrl.h"
#include "wx/sizer.h"
#include "wx/spinctrl.h"
#include "wx/stattext.h"
#include "gui/events.hh"
#include "gui/spin-ctrl.hh"
#include "util/convenience.hh"

namespace faint{

class FocusRelayingSpinCtrl : public wxSpinCtrl{
public:
  FocusRelayingSpinCtrl(wxWindow* parent, wxSize size) :
    wxSpinCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, size,
      wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER)
  {
    Bind(wxEVT_SET_FOCUS,
      [this](wxFocusEvent& focusEvent){
        wxCommandEvent newEvent(EVT_SET_FOCUS_ENTRY_CONTROL, wxID_ANY);
        newEvent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(newEvent);
        focusEvent.Skip();
      });

    Bind(wxEVT_KILL_FOCUS,
      [this](wxFocusEvent& focusEvent){
        wxCommandEvent newEvent(EVT_KILL_FOCUS_ENTRY_CONTROL, wxID_ANY);
        newEvent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(newEvent);
        focusEvent.Skip();
      });
  }
};

class FocusRelayingSpinCtrlDouble : public wxSpinCtrlDouble{
public:
  FocusRelayingSpinCtrlDouble(wxWindow* parent, wxSize size) :
    wxSpinCtrlDouble(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, size,
      wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER)
  {

    Bind(wxEVT_SET_FOCUS,
      [this](wxFocusEvent& focusEvent){
        wxCommandEvent newEvent(EVT_SET_FOCUS_ENTRY_CONTROL, wxID_ANY);
        newEvent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(newEvent);
        focusEvent.Skip();
      });

    Bind(wxEVT_KILL_FOCUS,
      [this](wxFocusEvent& focusEvent){
        wxCommandEvent newEvent(EVT_KILL_FOCUS_ENTRY_CONTROL, wxID_ANY);
        newEvent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(newEvent);
        focusEvent.Skip();
      });
  }
};

class IntSizeControl : public IntSettingCtrl{
public:
  IntSizeControl(wxWindow* parent, wxSize size, const IntSetting& setting, int value, const std::string& label)
    : IntSettingCtrl(parent, setting),
      m_changed(false)
  {
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    if (label.size() > 0){
      sizer->Add(new wxStaticText(this, wxID_ANY, label));
    }
    m_spinCtrl = new FocusRelayingSpinCtrl(this, size);
    m_spinCtrl->SetValue(value);
    m_spinCtrl->SetRange(1, 255);
    m_spinCtrl->SetBackgroundColour(wxColour(255, 255, 255));
    sizer->Add(m_spinCtrl, 0, wxALIGN_CENTER_HORIZONTAL);
    SetSizerAndFit(sizer);

    Bind(wxEVT_IDLE,
      [this](wxIdleEvent&){
        if (then_false(m_changed)){
          SendChangeEvent();
        }
      });


    Bind(wxEVT_SPINCTRL,
      [this](wxSpinEvent& event){
        event.Skip();
        SendChangeEvent();
      });

    Bind(wxEVT_TEXT_ENTER,
      [this](wxCommandEvent&){
        SendChangeEvent();
      });
  }

  int GetValue() const override{
    return m_spinCtrl->GetValue();
  }

  void SetValue(int value) override{
    m_spinCtrl->SetValue(value);
  }

private:
  bool m_changed;
  wxSpinCtrl* m_spinCtrl;
};

IntSettingCtrl* new_int_spinner(wxWindow* parent, const wxSize& size,
  const IntSetting& setting, int value, const std::string& label)
{
  return new IntSizeControl(parent, size, setting, value, label);
}

class FloatSizeControl : public FloatSettingControl{
public:
  FloatSizeControl(wxWindow* parent, wxSize size, const FloatSetting& setting,
    coord value, const std::string& label)
    : FloatSettingControl(parent, setting),
      m_changed(false)
  {
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    if (label.size() > 0){
      sizer->Add(new wxStaticText(this, wxID_ANY, label));
    }
    m_spinCtrl = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString,
      wxDefaultPosition, size, wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER);
    m_spinCtrl->SetValue(value);
    m_spinCtrl->SetRange(0.1, 255);
    m_spinCtrl->SetBackgroundColour(wxColour(255, 255, 255));

    sizer->Add(m_spinCtrl, 0, wxALIGN_CENTER_HORIZONTAL);
    SetSizerAndFit(sizer);

    Bind(wxEVT_IDLE,
      [this](wxIdleEvent&){
        if (then_false(m_changed)){
          SendChangeEvent();
        }
      });

    Bind(wxEVT_SPINCTRLDOUBLE,
      [this](wxSpinDoubleEvent& event){
        m_changed = true;
        event.Skip();
      });

    Bind(wxEVT_TEXT_ENTER,
      [this](wxCommandEvent&){
        SendChangeEvent();
      });
  }

  coord GetValue() const override{
    return m_spinCtrl->GetValue();
  }

  void SetValue(coord value) override{
    m_spinCtrl->SetValue(value);
  }

private:
  bool m_changed;
  wxSpinCtrlDouble* m_spinCtrl;
};

FloatSettingControl* new_float_spinner(wxWindow* parent, const wxSize& size,
  const FloatSetting& setting, coord value, const std::string& label)
{
  return new FloatSizeControl(parent, size, setting, value, label);
}

class SemiFloatSizeControl : public FloatSettingControl{
public:
  SemiFloatSizeControl(wxWindow* parent, wxSize size, const FloatSetting& setting,
    coord value, const std::string& label)
  : FloatSettingControl(parent, setting)
  {
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    if (label.size() > 0){
      sizer->Add(new wxStaticText(this, wxID_ANY, label));
    }
    m_spinCtrl = new FocusRelayingSpinCtrl(this, size);
    m_spinCtrl->SetValue(static_cast<int>(value));
    m_spinCtrl->SetRange(1, 255);
    m_spinCtrl->SetBackgroundColour(wxColour(255, 255, 255));
    m_lastValue = static_cast<int>(value);
    sizer->Add(m_spinCtrl, 0, wxALIGN_CENTER_HORIZONTAL);
    SetSizerAndFit(sizer);

    Bind(wxEVT_SPINCTRL,
      [this](wxSpinEvent& event){
        m_changed = true;
        event.Skip();
      });

    Bind(wxEVT_TEXT_ENTER,
      [this](wxCommandEvent&){
        m_lastValue = m_spinCtrl->GetValue();
        SendChangeEvent();
      });

    Bind(wxEVT_IDLE,
      [this](wxIdleEvent&){
        if (then_false(m_changed)){
          m_lastValue = m_spinCtrl->GetValue();
          SendChangeEvent();
        }
      });

    Bind(EVT_KILL_FOCUS_ENTRY_CONTROL,
      [this](wxEvent& event){
        event.Skip();
        if (m_lastValue  != m_spinCtrl->GetValue()){
          SendChangeEvent();
        }
      });
  }

  coord GetValue() const override{
    return static_cast<coord>(m_spinCtrl->GetValue());
  }

  virtual void SetValue(coord value) override{
    m_spinCtrl->SetValue(static_cast<int>(value));
  }

private:
  wxSpinCtrl* m_spinCtrl;
  bool m_changed;
  int m_lastValue;
};

FloatSettingControl* new_semi_float_spinner(wxWindow* parent, const wxSize& size,
  const FloatSetting& setting, coord value, const std::string& label)
{
  return new SemiFloatSizeControl(parent, size, setting, value, label);
}

} // namespace
