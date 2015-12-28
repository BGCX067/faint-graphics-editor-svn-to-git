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
#include "util/unique.hh"

template<void Func( faint::Bitmap& )>
class FunctionCommand : public Command {
 public:
  FunctionCommand( const std::string& name )
    : Command( CommandType::RASTER ),
      m_name(name)
  {}
  std::string Name() const override{
    return m_name;
  }
  void Do( CommandContext& context ) override{
    Func( context.GetRawBitmap() );
  }
private:
  std::string m_name;
};

template<typename T1, void Func( faint::Bitmap&, const T1& )>
class FunctionCommand1 : public Command {
 public:
  FunctionCommand1( const std::string& name, const T1& t1 )
    : Command( CommandType::RASTER ),
      m_name(name),
      m_t1( t1 )
  {}
  std::string Name() const override{
    return m_name;
  }

  void Do( CommandContext& context ) override{
    Func( context.GetRawBitmap(), m_t1 );
  }

private:
  std::string m_name;
  T1 m_t1;
};

// Forward that makes functions which do not take reference arguments compatible with
// FunctionCommand2
template<typename T1, typename T2, void Func( faint::Bitmap&, T1, T2)>
void unref_forwarder( faint::Bitmap& bmp, const T1& t1, const T2& t2){
  Func(bmp, t1, t2);
}

template<typename T1, void Func( faint::Bitmap&, T1)>
void unref_forwarder( faint::Bitmap& bmp, const T1& t1 ){
  Func(bmp, t1);
}

template<typename T1, typename T2, void Func( faint::Bitmap&, const T1&, const T2& )>
class FunctionCommand2 : public Command {
 public:
  FunctionCommand2( const std::string& name, const T1& t1, const T2& t2 )
    : Command( CommandType::RASTER ),
      m_name(name),
      m_t1( t1 ),
      m_t2( t2 )
  {}
  std::string Name() const override{
    return m_name;
  }

  void Do( CommandContext& context ) override{
    Func( context.GetRawBitmap(), m_t1, m_t2 );
  }
private:
  std::string m_name;
  T1 m_t1;
  T2 m_t2;
};

template<typename T1, typename T2, typename T3, void Func( faint::Bitmap&, const T1&, const T2&, const T3& )>
class FunctionCommand3 : public Command {
 public:
  FunctionCommand3( const std::string& name, const T1& t1, const T2& t2, const T3& t3 )
    : Command( CommandType::RASTER ),
      m_name(name),
      m_t1( t1 ),
      m_t2( t2 ),
      m_t3( t3 )
  {}
  std::string Name() const override{
    return m_name;
  }

  void Do( CommandContext& context ) override{
    Func( context.GetRawBitmap(), m_t1, m_t2, m_t3 );
  }
private:
  std::string m_name;
  T1 m_t1;
  T2 m_t2;
  T3 m_t3;
};

template<typename OBJ_T, typename T1, void Func( OBJ_T*, const T1& )>
class ObjFunctionCommand1 : public Command{
public:
  ObjFunctionCommand1( OBJ_T* object, const std::string& name,
    const typename Order<T1>::New& newArg,
    const typename Order<T1>::Old& oldArg )
    : Command( CommandType::OBJECT ),
      m_name(name),
      m_object(object),
      m_newArg(newArg.Get()),
      m_oldArg(oldArg.Get())
  {}

  void Do( CommandContext& ) override{
    Func(m_object, m_newArg);
  }

  std::string Name() const override{
    return m_name;
  }

  void Undo( CommandContext& ) override{
    Func(m_object, m_oldArg);
  }
private:
  std::string m_name;
  OBJ_T* m_object;
  T1 m_newArg;
  T1 m_oldArg;
};

template<typename OBJ_T, typename T1, typename T2, void Func( OBJ_T*, const T1&, const T2& )>
class ObjFunctionCommand2 : public Command{
public:
  ObjFunctionCommand2( OBJ_T* object, const std::string& name,
    const typename Order<T1>::New& new1,
    const typename Order<T2>::New& new2,
    const typename Order<T1>::Old& old1,
    const typename Order<T2>::Old& old2 )
    : Command( CommandType::OBJECT ),
      m_name(name),
      m_object(object),
      m_new1(new1.Get()),
      m_new2(new2.Get()),
      m_old1(old1.Get()),
      m_old2(old2.Get())
  {}

  void Do( CommandContext& ) override{
    Func(m_object, m_new1, m_new2);
  }

  std::string Name() const override{
    return m_name;
  }

  void Undo( CommandContext& ) override{
    Func(m_object, m_old1, m_old2);
  }
private:
  std::string m_name;
  OBJ_T* m_object;
  T1 m_new1;
  T2 m_new2;
  T1 m_old1;
  T1 m_old2;
};

#endif
