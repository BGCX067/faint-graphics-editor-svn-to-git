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

#ifndef FAINT_TOOL_SETTING_CTRL_HH
#define FAINT_TOOL_SETTING_CTRL_HH
#include "wx/panel.h"
#include "wx/event.h"
#include "gui/setting-events.hh"


namespace faint{

// Event for notifying that a text entry control has received or lost
// focus (used to disable some Python binds)
extern const wxEventType SET_FOCUS_ENTRY_CONTROL;
extern const wxEventTypeTag<wxCommandEvent> EVT_SET_FOCUS_ENTRY_CONTROL;

extern const wxEventType KILL_FOCUS_ENTRY_CONTROL;
extern const wxEventTypeTag<wxCommandEvent> EVT_KILL_FOCUS_ENTRY_CONTROL;

class ToolSettingCtrl : public wxPanel{
public:
  ToolSettingCtrl(wxWindow* parent);
  virtual void SendChangeEvent() = 0;
  virtual bool UpdateControl(const Settings&) = 0;
};

class IntSettingCtrl : public ToolSettingCtrl{
public:
  IntSettingCtrl(wxWindow* parent, const IntSetting&);
  virtual IntSetting GetSetting() const;
  virtual int GetValue() const = 0;
  virtual void SetValue(int) = 0;
  virtual bool UpdateControl(const Settings&) override;
  virtual void SendChangeEvent() override;
private:
  IntSetting m_setting;
};

class FloatSettingControl : public ToolSettingCtrl{
public:
  FloatSettingControl(wxWindow* parent, const FloatSetting&);
  virtual FloatSetting GetSetting() const;
  virtual coord GetValue() const = 0;
  virtual void SetValue(coord) = 0;
  virtual bool UpdateControl(const Settings&) override;
  virtual void SendChangeEvent() override;
private:
  FloatSetting m_setting;
};

class BoolSettingControl : public ToolSettingCtrl{
public:
  BoolSettingControl(wxWindow* parent, const BoolSetting&);
  virtual BoolSetting GetSetting() const;
  virtual bool GetValue() const = 0;
  virtual void SetValue(bool) = 0;
  virtual bool UpdateControl(const Settings&) override;
  virtual void SendChangeEvent() override;
private:
  BoolSetting m_setting;
};

} // namespace

#endif
