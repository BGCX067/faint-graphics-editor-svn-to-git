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

int NewSettingId(){
  static int max_id = 1003;
  return max_id++;
}

// Implement all Set-variants
#define SETTERDEFI( SettingType )\
  template<> void FaintSettings::Set( const SettingType& s, const SettingType::ValueType& v ){ \
    m_##SettingType##Map[ s ] =  v; }

SETTERDEFI( BoolSetting )
SETTERDEFI( ColorSetting )
SETTERDEFI( IntSetting )
SETTERDEFI( StrSetting )
SETTERDEFI( FloatSetting )

// Implement all Get-variants
#define GETTERDEFI( SettingType )					\
  template<> const SettingType::ValueType& FaintSettings::Get( const SettingType& s ) const { \
    return m_##SettingType##Map.find( s )->second; }
GETTERDEFI( BoolSetting )
GETTERDEFI( ColorSetting )
GETTERDEFI( IntSetting )
GETTERDEFI( StrSetting )
GETTERDEFI( FloatSetting )

// Implement Get-variants which returns a default value for missing settings
#define DEFAULTGETTERDEFI( SettingType )\
  template<> const SettingType::ValueType& FaintSettings::GetDefault( const SettingType& s, const SettingType::ValueType& defaultValue ) const { \
    SETTMAP( SettingType )::const_iterator it = m_##SettingType##Map.find( s ); \
    return ( it == m_##SettingType##Map.end() ) ? defaultValue : it->second; \
  }

DEFAULTGETTERDEFI( BoolSetting )
DEFAULTGETTERDEFI( ColorSetting )
DEFAULTGETTERDEFI( IntSetting )
DEFAULTGETTERDEFI( StrSetting )
DEFAULTGETTERDEFI( FloatSetting )

template<>
bool FaintSettings::Has( const IntSetting& setting ) const{
  return ( m_IntSettingMap.find( setting ) != m_IntSettingMap.end() );
}

template<>
bool FaintSettings::Has( const StrSetting& setting ) const{
  return ( m_StrSettingMap.find( setting ) != m_StrSettingMap.end() );
}

template<>
bool FaintSettings::Has( const BoolSetting& setting ) const{
  return ( m_BoolSettingMap.find( setting ) != m_BoolSettingMap.end() );
}

template<>
bool FaintSettings::Has( const ColorSetting& setting ) const{
  return ( m_ColorSettingMap.find( setting ) != m_ColorSettingMap.end() );
}

template<>
bool FaintSettings::Has( const FloatSetting& setting ) const{
  return ( m_FloatSettingMap.find( setting ) != m_FloatSettingMap.end() );
}

template<>
void FaintSettings::Erase( const IntSetting& setting ){
  m_IntSettingMap.erase( setting );
}

template<>
void FaintSettings::Erase( const StrSetting& setting ){
  m_StrSettingMap.erase( setting );
}

template<>
void FaintSettings::Erase( const BoolSetting& setting ){
  m_BoolSettingMap.erase( setting );
}

template<>
void FaintSettings::Erase( const FloatSetting& setting ){
  m_FloatSettingMap.erase( setting );
}

template<>
void FaintSettings::Erase( const ColorSetting& setting ){
  m_ColorSettingMap.erase( setting );
}

bool FaintSettings::Has( const UntypedSetting& s ) const{
  return Has( IntSetting( s.ToInt() ) ) ||
    Has( StrSetting( s.ToInt() ) ) ||
    Has( BoolSetting( s.ToInt() ) ) ||
    Has( ColorSetting( s.ToInt() ) ) ||
    Has( FloatSetting( s.ToInt() ) );
}

bool FaintSettings::Update( const FaintSettings& other ){
  bool updated = false;
  for ( SETTMAP( IntSetting )::iterator it = m_IntSettingMap.begin();
        it != m_IntSettingMap.end(); ++it )
    {
      if ( other.Has( it->first ) ){
        it->second = other.Get( it->first );
        updated = true;
      }
    }

  for ( SETTMAP( StrSetting )::iterator it = m_StrSettingMap.begin();
        it != m_StrSettingMap.end(); ++it )
    {
      if ( other.Has( it->first ) ){
        it->second = other.Get( it->first );
        updated = true;
      }
    }

  for ( SETTMAP( ColorSetting )::iterator it = m_ColorSettingMap.begin();
        it != m_ColorSettingMap.end(); ++it )
    {
      if ( other.Has( it->first ) ){
        it->second = other.Get( it->first );
        updated = true;
      }
    }

  for ( SETTMAP( BoolSetting )::iterator it = m_BoolSettingMap.begin();
        it != m_BoolSettingMap.end(); ++it )
    {
      if ( other.Has( it->first ) ){
        it->second = other.Get( it->first );
        updated = true;
      }
    }

  for ( SETTMAP( FloatSetting )::iterator it = m_FloatSettingMap.begin();
        it != m_FloatSettingMap.end(); ++it )
    {
      if ( other.Has( it->first ) ){
        it->second = other.Get( it->first );
        updated = true;
      }
    }

  return updated;
}

bool FaintSettings::WouldUpdate( const FaintSettings& other ) const{
  for ( SETTMAP( IntSetting )::const_iterator it = m_IntSettingMap.begin(); it != m_IntSettingMap.end(); ++it ) {
    if ( other.Has( it->first ) ){
      IntSetting::ValueType value( other.Get( it->first ) );
      if ( it->second != value ){
        return true;
      }
    }
  }

  for ( SETTMAP( StrSetting )::const_iterator it = m_StrSettingMap.begin(); it != m_StrSettingMap.end(); ++it ) {
    if ( other.Has( it->first ) ){
      StrSetting::ValueType value( other.Get( it->first ) );
      if ( it->second != value ){
        return true;
      }
    }
  }

  for ( SETTMAP( ColorSetting )::const_iterator it = m_ColorSettingMap.begin(); it != m_ColorSettingMap.end(); ++it ) {
    if ( other.Has( it->first ) ){
      ColorSetting::ValueType value( other.Get( it->first ) );
      if ( it->second != value ){
        return true;
      }
    }
  }


  for ( SETTMAP( BoolSetting )::const_iterator it = m_BoolSettingMap.begin(); it != m_BoolSettingMap.end(); ++it ) {
    if ( other.Has( it->first ) ){
      BoolSetting::ValueType value( other.Get( it->first ) );
      if ( it->second != value ){
        return true;
      }
    }
  }

  for ( SETTMAP( FloatSetting )::const_iterator it = m_FloatSettingMap.begin(); it != m_FloatSettingMap.end(); ++it ) {
    if ( other.Has( it->first ) ){
      FloatSetting::ValueType value( other.Get( it->first ) );
      if ( it->second != value ){
        return true;
      }
    }
  }
  return false;
}

std::vector<UntypedSetting> FaintSettings::GetRaw() const{
  std::vector<UntypedSetting> settings;
  for ( SETTMAP( BoolSetting )::const_iterator it = m_BoolSettingMap.begin(); it != m_BoolSettingMap.end(); ++it ){
    settings.push_back( UntypedSetting(it->first) );
  }
  for ( SETTMAP( IntSetting )::const_iterator it = m_IntSettingMap.begin(); it != m_IntSettingMap.end(); ++it ){
    settings.push_back( UntypedSetting(it->first) );
  }
  for ( SETTMAP( StrSetting )::const_iterator it = m_StrSettingMap.begin(); it != m_StrSettingMap.end(); ++it ){
    settings.push_back( UntypedSetting(it->first) );
  }
  for ( SETTMAP( ColorSetting )::const_iterator it = m_ColorSettingMap.begin(); it != m_ColorSettingMap.end(); ++it ){
    settings.push_back( UntypedSetting(it->first) );
  }
  for ( SETTMAP( FloatSetting )::const_iterator it = m_FloatSettingMap.begin(); it != m_FloatSettingMap.end(); ++it ){
    settings.push_back( UntypedSetting(it->first) );
  }

  return settings;
}

void FaintSettings::UpdateAll( const FaintSettings& other ){
  m_BoolSettingMap.insert( other.m_BoolSettingMap.begin(), other.m_BoolSettingMap.end() );
  m_IntSettingMap.insert( other.m_IntSettingMap.begin(), other.m_IntSettingMap.end() );
  m_StrSettingMap.insert( other.m_StrSettingMap.begin(), other.m_StrSettingMap.end() );
  m_ColorSettingMap.insert( other.m_ColorSettingMap.begin(), other.m_ColorSettingMap.end() );
  m_FloatSettingMap.insert( other.m_FloatSettingMap.begin(), other.m_FloatSettingMap.end() );
}

void FaintSettings::Clear(){
  m_BoolSettingMap.clear();
  m_IntSettingMap.clear();
  m_StrSettingMap.clear();
  m_ColorSettingMap.clear();
  m_FloatSettingMap.clear();
}

BoolSetting::ValueType FaintSettings::Not( const BoolSetting& s ) const{
  return ! ( m_BoolSettingMap.find( s )->second );
}
