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

#include "settings.hh"

setting_id new_setting_id(){
  static int max_id = 1003;
  return max_id++;
}

// Implement all Set-variants
#define SETTERDEFI( SettingType )\
  template<> void Settings::Set( const SettingType& s, const SettingType::ValueType& v ){ \
    m_##SettingType##Map[ s ] =  v; }

SETTERDEFI( BoolSetting )
SETTERDEFI( ColorSetting )
SETTERDEFI( IntSetting )
SETTERDEFI( StrSetting )
SETTERDEFI( FloatSetting )

// Implement all Get-variants
#define GETTERDEFI( SettingType ) \
template<> const SettingType::ValueType& Settings::Get( const SettingType& s ) const { \
  auto it = m_##SettingType##Map.find( s ); \
  assert( it != m_##SettingType##Map.end() ); \
  return it->second; \
} // End of macro


GETTERDEFI( BoolSetting )
GETTERDEFI( ColorSetting )
GETTERDEFI( IntSetting )
GETTERDEFI( StrSetting )
GETTERDEFI( FloatSetting )

// Implement Get-variants which returns a default value for missing settings
#define DEFAULTGETTERDEFI( SettingType )\
template<> const SettingType::ValueType& Settings::GetDefault( const SettingType& s, const SettingType::ValueType& defaultValue ) const { \
  auto it = m_##SettingType##Map.find( s ); \
  return ( it == m_##SettingType##Map.end() ) ? defaultValue : it->second; \
} // End of macro

DEFAULTGETTERDEFI( BoolSetting )
DEFAULTGETTERDEFI( ColorSetting )
DEFAULTGETTERDEFI( IntSetting )
DEFAULTGETTERDEFI( StrSetting )
DEFAULTGETTERDEFI( FloatSetting )

template<>
bool Settings::Has( const IntSetting& setting ) const{
  return ( m_IntSettingMap.find( setting ) != m_IntSettingMap.end() );
}

template<>
bool Settings::Has( const StrSetting& setting ) const{
  return ( m_StrSettingMap.find( setting ) != m_StrSettingMap.end() );
}

template<>
bool Settings::Has( const BoolSetting& setting ) const{
  return ( m_BoolSettingMap.find( setting ) != m_BoolSettingMap.end() );
}

template<>
bool Settings::Has( const ColorSetting& setting ) const{
  return ( m_ColorSettingMap.find( setting ) != m_ColorSettingMap.end() );
}

template<>
bool Settings::Has( const FloatSetting& setting ) const{
  return ( m_FloatSettingMap.find( setting ) != m_FloatSettingMap.end() );
}

template<>
void Settings::Erase( const IntSetting& setting ){
  m_IntSettingMap.erase( setting );
}

template<>
void Settings::Erase( const StrSetting& setting ){
  m_StrSettingMap.erase( setting );
}

template<>
void Settings::Erase( const BoolSetting& setting ){
  m_BoolSettingMap.erase( setting );
}

template<>
void Settings::Erase( const FloatSetting& setting ){
  m_FloatSettingMap.erase( setting );
}

template<>
void Settings::Erase( const ColorSetting& setting ){
  m_ColorSettingMap.erase( setting );
}

bool Settings::Has( const UntypedSetting& s ) const{
  return Has( IntSetting( s.ToInt() ) ) ||
    Has( StrSetting( s.ToInt() ) ) ||
    Has( BoolSetting( s.ToInt() ) ) ||
    Has( ColorSetting( s.ToInt() ) ) ||
    Has( FloatSetting( s.ToInt() ) );
}

template<typename T>
bool update_map_values( T& targetMap, const Settings& source ){
  // Updatate the values of all settings that exist in both targetMap
  // and source with the values from source.
  bool updated = false;
  for ( auto& targetItem : targetMap ){
    const auto& settingId = targetItem.first;
    if ( source.Has( settingId ) ){
      targetItem.second = source.Get(settingId);
      updated = true;
    }
  }
  return updated;
}

bool Settings::Update( const Settings& other ){
  return update_map_values( m_BoolSettingMap, other ) |
    update_map_values( m_ColorSettingMap, other) |
    update_map_values( m_FloatSettingMap, other) |
    update_map_values( m_IntSettingMap, other) |
    update_map_values( m_StrSettingMap, other);
}


template<typename T>
void append_as_untyped( std::vector<UntypedSetting>& untypedIds, const T& map ){
  for ( auto& item : map ){
    untypedIds.push_back(UntypedSetting(item.first));
  }
}

std::vector<UntypedSetting> Settings::GetRaw() const{
  std::vector<UntypedSetting> untyped;
  append_as_untyped(untyped, m_BoolSettingMap);
  append_as_untyped(untyped, m_IntSettingMap);
  append_as_untyped(untyped, m_StrSettingMap);
  append_as_untyped(untyped, m_ColorSettingMap);
  append_as_untyped(untyped, m_FloatSettingMap);
  return untyped;
}

template<typename T>
void update_map_content( T& targetMap, const T& otherMap ){
  // Set the value of all settings from otherMap in targetMap (add the
  // settings if they do not exist).
  for ( const auto& item : otherMap ){
    targetMap[item.first] = item.second;
  }
}

void Settings::UpdateAll( const Settings& other ){
  update_map_content(m_BoolSettingMap, other.m_BoolSettingMap);
  update_map_content(m_ColorSettingMap, other.m_ColorSettingMap);
  update_map_content(m_FloatSettingMap, other.m_FloatSettingMap);
  update_map_content(m_IntSettingMap, other.m_IntSettingMap);
  update_map_content(m_StrSettingMap, other.m_StrSettingMap);
}

void Settings::Clear(){
  m_BoolSettingMap.clear();
  m_ColorSettingMap.clear();
  m_FloatSettingMap.clear();
  m_IntSettingMap.clear();
  m_StrSettingMap.clear();
}

BoolSetting::ValueType Settings::Not( const BoolSetting& s ) const{
  auto it = m_BoolSettingMap.find(s);
  assert(it != m_BoolSettingMap.end());
  return !(it->second);
}

SettingNotifier::~SettingNotifier(){
}

class NullNotifier : public SettingNotifier {
public:
  virtual void Notify( const BoolSetting&, bool ) override {};
  virtual void Notify( const IntSetting&, int ) override {};
  virtual void Notify( const StrSetting&, const std::string& ) override {}
  virtual void Notify( const ColorSetting&, const faint::DrawSource& ) override{}
  virtual void Notify( const FloatSetting&, FloatSetting::ValueType ) override{}
};

NullNotifier g_nullNotifier;

SettingNotifier& get_null_notifier(){
  return g_nullNotifier;
}
