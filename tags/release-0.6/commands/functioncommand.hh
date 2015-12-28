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

#ifndef FAINT_FUNCTIONCOMMAND_HH
#define FAINT_FUNCTIONCOMMAND_HH
#include "commands/command.hh"

template<void Func( faint::Bitmap& )>
class FunctionCommand : public Command {
 public:
  FunctionCommand( const std::string& name )
    : Command( CMD_TYPE_RASTER ),
      m_name(name)
  {}
  std::string Name() const{
    return m_name;
  }
  void Do( CommandContext& context ){
    Func( context.GetRawBitmap() );
  }
private:
  std::string m_name;
};

template<typename T1, void Func( faint::Bitmap&, const T1& )>
class FunctionCommand1 : public Command {
 public:
  FunctionCommand1( const std::string& name, const T1& t1 )
  : Command( CMD_TYPE_RASTER ),
    m_name(name),
    m_t1( t1 )
  {}
  std::string Name() const{
    return m_name;
  }

  void Do( CommandContext& context ){
    Func( context.GetRawBitmap(), m_t1 );
  }

private:
  std::string m_name;
  T1 m_t1;
};


template<typename T1, typename T2, void Func( faint::Bitmap&, const T1&, const T2& )>
class FunctionCommand2 : public Command {
 public:
  FunctionCommand2( const std::string& name, const T1& t1, const T2& t2 )
  : Command( CMD_TYPE_RASTER ),
    m_name(name),
    m_t1( t1 ),
    m_t2( t2 )
  {}
  std::string Name() const{
    return m_name;
  }

  void Do( CommandContext& context ){
    Func( context.GetRawBitmap(), m_t1, m_t2 );
  }
private:
  std::string m_name;
  T1 m_t1;
  T2 m_t2;
};

#endif
