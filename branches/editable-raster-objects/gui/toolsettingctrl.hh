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

#ifndef FAINT_TOOLSETTINGCTRL_HH
#define FAINT_TOOLSETTINGCTRL_HH
#include "wx/panel.h"
#include "wx/event.h"
#include "util/settings.hh"

DECLARE_EVENT_TYPE(EVT_SETTING_CHANGE, -1)
DECLARE_EVENT_TYPE( EVT_SET_FOCUS_ENTRY_CONTROL, -1 )
DECLARE_EVENT_TYPE( EVT_KILL_FOCUS_ENTRY_CONTROL, -1 )

class ToolSettingCtrl : public wxPanel{
public:
  ToolSettingCtrl( wxWindow* parent );
  virtual UntypedSetting GetUntypedSetting() const = 0;
  virtual void Notify( SettingNotifier& ) = 0;
  void SendChangeEvent();
  virtual bool UpdateControl( const Settings& ) = 0;
};

class IntSettingCtrl : public ToolSettingCtrl{
public:
  IntSettingCtrl( wxWindow* parent, const IntSetting& );
  virtual IntSetting GetSetting() const;
  virtual UntypedSetting GetUntypedSetting() const override;
  virtual int GetValue() const = 0;
  virtual void Notify( SettingNotifier& ) override;
  virtual void SetValue( int ) = 0;
  virtual bool UpdateControl( const Settings& ) override;
private:
  IntSetting m_setting;
};

class FloatSettingControl : public ToolSettingCtrl{
public:
  FloatSettingControl( wxWindow* parent, const FloatSetting& );
  virtual FloatSetting GetSetting() const;
  virtual UntypedSetting GetUntypedSetting() const override;
  virtual faint::coord GetValue() const = 0;
  virtual void SetValue( faint::coord ) = 0;
  virtual bool UpdateControl( const Settings& ) override;
  virtual void Notify( SettingNotifier& ) override;
private:
  FloatSetting m_setting;
};

class BoolSettingControl : public ToolSettingCtrl{
public:
  BoolSettingControl( wxWindow* parent, const BoolSetting& );
  virtual BoolSetting GetSetting() const;
  virtual UntypedSetting GetUntypedSetting() const override;
  virtual bool GetValue() const = 0;
  virtual void Notify( SettingNotifier& ) override;
  virtual void SetValue( bool ) = 0;
  virtual bool UpdateControl( const Settings& ) override;
private:
  BoolSetting m_setting;
};

#endif
