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

#include "toolsettingctrl.hh"

// Event signalling a setting change from within a ToolSettingCtrl
DEFINE_EVENT_TYPE(EVT_SETTING_CHANGE)

// Events for text-entry control focus changes, to allow disabling
// shortcuts and binds that would interfere with entry
DEFINE_EVENT_TYPE( EVT_SET_FOCUS_ENTRY_CONTROL )
DEFINE_EVENT_TYPE( EVT_KILL_FOCUS_ENTRY_CONTROL )

ToolSettingCtrl::ToolSettingCtrl( wxWindow* parent )
  : wxPanel(parent)
{}

void ToolSettingCtrl::SendChangeEvent(){
  wxCommandEvent event(EVT_SETTING_CHANGE, GetId());
  event.SetEventObject(this);
  GetEventHandler()->ProcessEvent( event );
}

IntSettingCtrl::IntSettingCtrl( wxWindow* parent, const IntSetting& s )
  : ToolSettingCtrl( parent),
    m_setting(s)
{}

IntSetting IntSettingCtrl::GetSetting() const{
  return m_setting;
}

UntypedSetting IntSettingCtrl::GetUntypedSetting() const {
  return UntypedSetting( m_setting );
}

bool IntSettingCtrl::UpdateControl( const Settings& s ){
  if ( s.Has( m_setting ) ){
    SetValue( s.Get( m_setting ) );
    return true;
  }
  return false;
}

void IntSettingCtrl::Notify( SettingNotifier& notifier ){
  notifier.Notify( m_setting, GetValue() );
}


FloatSettingControl::FloatSettingControl( wxWindow* parent, const FloatSetting& s )
  : ToolSettingCtrl( parent ),
  m_setting(s)
{}

FloatSetting FloatSettingControl::GetSetting() const{
  return m_setting;
}

UntypedSetting FloatSettingControl::GetUntypedSetting() const{
  return UntypedSetting( m_setting );
}

bool FloatSettingControl::UpdateControl( const Settings& s ){
  if ( s.Has( m_setting ) ){
    SetValue( s.Get( m_setting ) );
    return true;
  }
  return false;
}

void FloatSettingControl::Notify( SettingNotifier& notifier ){
  notifier.Notify( m_setting, GetValue() );
}

BoolSettingControl::BoolSettingControl( wxWindow* parent, const BoolSetting& s )
  : ToolSettingCtrl(parent),
    m_setting(s)
{}

BoolSetting BoolSettingControl::GetSetting() const{
  return m_setting;
}

UntypedSetting BoolSettingControl::GetUntypedSetting() const{
  return UntypedSetting(m_setting);
}

bool BoolSettingControl::UpdateControl( const Settings& s ){
  if ( s.Has( m_setting ) ){
    SetValue( s.Get( m_setting ) );
    return true;
  }
  return false;
}

void BoolSettingControl::Notify( SettingNotifier& notifier ){
  notifier.Notify( m_setting, GetValue() );
}
