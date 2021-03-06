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

#ifndef FAINT_SETTINGS_HH
#define FAINT_SETTINGS_HH
#include <map>
#include <vector>
#include "bitmap/paint.hh"
#include "geo/primitive.hh"
#include "text/utf8-string.hh"

namespace faint{

class BoundSetting;

using setting_id = int;
setting_id new_setting_id();

template<typename VAL_T>
class Setting{
public:
  Setting();
  explicit Setting(setting_id);
  bool operator<(const Setting<VAL_T>& other) const;
  bool operator==(const Setting<VAL_T>& other) const;
  int ToInt() const;
  using ValueType = VAL_T;
private:
  setting_id m_id;
};

template<typename T>
Setting<T>::Setting(){
  m_id = new_setting_id();
}

template<typename T>
Setting<T>::Setting(int id){
  m_id = id;
}

template <typename T>
bool Setting<T>::operator<(const Setting<T>& other) const{
  return m_id < other.m_id;
}

template <typename T>
bool Setting<T>::operator==(const Setting<T>& other) const{
  return m_id == other.m_id;
}

template<typename T>
int Setting<T>::ToInt() const{
  return m_id;
}

using BoolSetting = Setting<bool>;
using IntSetting = Setting<int>;
using StrSetting = Setting<utf8_string>;
using FloatSetting = Setting<coord>;
using ColorSetting = Setting<Paint>;

template<typename ENUM_T>
class EnumSetting{
public:
  EnumSetting(){
    m_id = new_setting_id();
  }

  explicit EnumSetting(setting_id id){
    m_id = id;

  }
  bool operator<(const EnumSetting& other) const{
    return m_id < other.m_id;
  }
  bool operator==(const EnumSetting& other) const{
    return m_id == other.m_id;
  }
  operator IntSetting() const{
    return IntSetting(m_id);
  }
  int ToInt() const{
    return m_id;
  }
  using ValueType = ENUM_T;
private:
  setting_id m_id;
};

class UntypedSetting{
public:
  explicit UntypedSetting(setting_id id)
    : m_id(id)
  {}
  template<typename T>
  explicit UntypedSetting(const Setting<T>& s){
    m_id = s.ToInt();
  }
  template<typename T>
  bool operator<(const Setting<T>& other) const{
    return m_id < other.ToInt();
  }

  template<typename T>
  bool operator==(const Setting<T>& other) const{
    return m_id == other.ToInt();
  }

  template<typename T>
  bool operator==(const EnumSetting<T>& other) const{
    return m_id == other.ToInt();
  }

  int ToInt() const{
    return m_id;
  }
private:
  UntypedSetting(); // Silence cppcheck warning
  int m_id;
};

template<typename T>
UntypedSetting untyped(const Setting<T>& setting){
  return UntypedSetting(setting);
}

template<typename T>
UntypedSetting untyped(const EnumSetting<T>& setting){
  return UntypedSetting(setting.ToInt());
}

template<typename T>
bool operator==(const IntSetting& lhs, const EnumSetting<T>& rhs){
  return lhs.ToInt() == rhs.ToInt();
}

#define SETTMAP(SettingType) std::map< SettingType, SettingType::ValueType >

class Settings{
public:
  Settings();
  template<typename T>
  void Set(const Setting<T>& s, const typename Setting<T>::ValueType& val);

  template<typename T>
  void Set(const EnumSetting<T>& s, const typename EnumSetting<T>::ValueType& val){
    Set(static_cast<IntSetting>(s), to_int(val));
  }

  template<typename T>
  const typename Setting<T>::ValueType& Get(const Setting<T>&) const;

  template<typename T>
  typename EnumSetting<T>::ValueType Get(const EnumSetting<T>& setting) const{
    return static_cast<typename EnumSetting<T>::ValueType>(Get(IntSetting(setting)));
  }

  template<typename T>
  const typename Setting<T>::ValueType& GetDefault(const Setting<T>&, const typename Setting<T>::ValueType& defaultValue) const;

  template<typename T>
  typename EnumSetting<T>::ValueType GetDefault(const EnumSetting<T>& setting, const typename EnumSetting<T>::ValueType defaultValue) const{
    if (Has(setting)){
      return Get(setting);
    }
    return defaultValue;
  }

  BoolSetting::ValueType Not(const BoolSetting&) const;

  template<typename T>
  bool Has(const Setting<T>&) const;
  template<typename T>
  bool Has(const EnumSetting<T>& s) const{
    return Has(static_cast<IntSetting>(s));
  }
  bool Has(const UntypedSetting&) const;

  template<typename T>
  bool Lacks(const Setting<T>& s) const{
    return !Has(s);
  }

  template<typename T>
  bool Lacks(const EnumSetting<T>& s) const{
    return !Has(s);
  }

  template<typename T>
  void Erase(const Setting<T>&);

  template<typename T>
  void Erase(const EnumSetting<T>& s){
    return Erase(static_cast<IntSetting>(s));
  }

  std::vector<UntypedSetting> GetRaw() const;

  // Updates these settings with values from those settings
  bool Update(const Settings&);

  // Updates these settings with values from that setting,
  // if these settings already manage that setting.
  bool Update(const BoundSetting&);

  // Copies all those settings into these settings.
  void UpdateAll(const Settings&);
  void Clear();
  bool Empty() const;
private:
  SETTMAP(BoolSetting) m_BoolSettingMap;
  SETTMAP(IntSetting) m_IntSettingMap;
  SETTMAP(StrSetting) m_StrSettingMap;
  SETTMAP(ColorSetting) m_ColorSettingMap;
  SETTMAP(FloatSetting) m_FloatSettingMap;
};

// Declare specializations of the Set-member function template
#define SETTERDECL(SettingType)\
  template<> void Settings::Set(const SettingType& s, const SettingType::ValueType& v);

SETTERDECL(BoolSetting)
SETTERDECL(ColorSetting)
SETTERDECL(IntSetting)
SETTERDECL(StrSetting)
SETTERDECL(FloatSetting)

// Declare specializations of the Get-member function template
#define GETTERDECL(SettingType) \
  template<> const SettingType::ValueType& Settings::Get(const SettingType& s) const;

GETTERDECL(BoolSetting)
GETTERDECL(ColorSetting)
GETTERDECL(IntSetting)
GETTERDECL(StrSetting)
GETTERDECL(FloatSetting)

// Declare specializations of the GetDefault-member function template
#define DEFAULTGETTERDECL(SettingType) \
  template<> const SettingType::ValueType& Settings::GetDefault(const SettingType&, const SettingType::ValueType& defaultValue) const;

DEFAULTGETTERDECL(BoolSetting)
DEFAULTGETTERDECL(ColorSetting)
DEFAULTGETTERDECL(IntSetting)
DEFAULTGETTERDECL(StrSetting)
DEFAULTGETTERDECL(FloatSetting)

template<typename T>
Settings without(const Settings& settings, const T& excluded){
  if (!settings.Has(excluded)){
    return settings;
  }
  Settings s2(settings);
  s2.Erase(excluded);
  return s2;
}

template<typename T1, typename T2>
Settings without(const Settings& settings, const T1& excl1, const T2& excl2){
  Settings s2(settings);
  s2.Erase(excl1);
  s2.Erase(excl2);
  return s2;
}

template<typename T1>
Settings with(const Settings& settings, const T1& k, const typename T1::ValueType& v){
  Settings s2(settings);
  s2.Set(k, v);
  return s2;
}

} // namespace

#endif
