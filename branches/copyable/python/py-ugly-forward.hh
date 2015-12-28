// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#ifndef FAINT_PY_UGLY_FORWARD_HH
#define FAINT_PY_UGLY_FORWARD_HH
#include "python/py-function-error.hh"
#include "python/py-parse.hh"
namespace faint{

class PythonErrorReturn{
public:
  operator int() const{
    return init_fail;
  }
  operator PyObject*() const{
    return nullptr;
  }
};

// Defined in py-util.cpp
PyObject* set_error(const PythonError&);


// Helper macro for the forwarding structures.
// Resolves the C++-type of "self" (i.e. the first argument for a
// C-Python member function).
//
// Requires a template specialization of MappedType that maps the
// Python object structure to the C++ class.
#define GET_TYPED_SELF(CLASS_T, obj) static_assert(!std::is_same<typename MappedType<CLASS_T>::PYTHON_TYPE, InvalidMappedType>::value, "MappedType not specialized for CLASS_T"); \
  typename MappedType<CLASS_T>::PYTHON_TYPE* self = (typename MappedType<CLASS_T>::PYTHON_TYPE*)obj

// -----------------------------------------------------------------------
// Forwarding structures
// -----------------------------------------------------------------------
// A bunch of structures which all contain a function (call_func) follow.
//
// This function:
// 1. makes a C++-function appear like a PyMethodDef-compatible
//    PyObject*(PyObject*,PyObject*)-function
// 2. turns the PyObject*-self into the strongly typed C++-class expected
//    as the first argument of the C++ function
// 3. turns the PyObject* argument list into the expected C++-arguments
// 4. calls the C++ function if the arguments could be successfully
//    resolved by the wrapper (call_func).
// 5. Handles exceptions or turns the strongly typed return into
//    a PyObject*
//
// The variants of the structures handle different numbers of
// arguments and whether the function has a (typed) return or void.

template<typename CLASS_T>
struct regular_t{
  // The simplest argument resolver, provides only the type of the class
  // (and checks it for errors). The arguments will not be resolved
  // (i.e. passed as a PyObject*). Also returns a a mere PyObject*.

  template<PyObject* Func(CLASS_T, PyObject*)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* args){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    try{
      return Func(get_cpp_object(self), args);
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<class A, class ...B>
bool wrapped_parse_args(A& head, B&... tail){
  // Catches exceptions from parse_args and sets
  // them as Python errors.
  try{
    return parse_args(head, tail...);
  }
  catch (const PythonError& error){
    set_error(error);
    return false;
  }
}

template<typename CLASS_T>
struct regular_void_t{
  // Unparsed arguments, void return specialization.
  // Same as regular_t, but no return (void).

  template<void Func(CLASS_T, PyObject*)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* args){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }

    try{
      Func(get_cpp_object(self), args);
      return Py_BuildValue("");
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename CLASS_T, typename RET>
struct zero_arg_t{
  // Zero arguments

  template<RET Func(CLASS_T)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* /*args*/){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    try{
      return build_result(Func(get_cpp_object(self)));
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename CLASS_T>
struct zero_arg_t<CLASS_T, void>{
  // Zero arguments, void-return specialization

  template<void Func(CLASS_T)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* /*args*/){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    try{
      Func(get_cpp_object(self));
      return Py_BuildValue("");
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename CLASS_T, typename T1, typename RET>
struct one_arg_t{
  // One argument

  template<RET Func(CLASS_T, const T1&)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* args){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    T1 a1;
    if (!wrapped_parse_args(a1, args)){
      return nullptr;
    }
    try{
      return build_result(Func(get_cpp_object(self), a1));
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename CLASS_T, typename T1>
struct one_arg_t<CLASS_T, T1, void>{
  // One argument, void-return specialization

  template<void Func(CLASS_T, const T1&)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* args){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    T1 a1;
    if (!wrapped_parse_args(a1, args)){
      return nullptr;
    }
    try{
      Func(get_cpp_object(self), a1);
      return Py_BuildValue("");
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename CLASS_T, typename T1, typename T2, typename RET>
struct two_arg_t{
  // Two arguments

  template<RET Func(CLASS_T, const T1&, const T2&)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* args){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    T1 a1;
    T2 a2;
    if (!wrapped_parse_args(a1, a2, args)){
      return nullptr;
    }
    try{
      return build_result(Func(get_cpp_object(self), a1, a2));
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError& error){
      return nullptr;
    }
  }
};

template<typename CLASS_T, typename T1, typename T2>
struct two_arg_t<CLASS_T, T1, T2, void>{
  // Two arguments, void return type specialization

  template<void Func(CLASS_T, const T1&, const T2&)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* args){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    T1 a1;
    T2 a2;
    if (!wrapped_parse_args(a1, a2, args)){
      return nullptr;
    }
    try{
      Func(get_cpp_object(self), a1, a2);
      return Py_BuildValue("");
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename CLASS_T, typename T1, typename T2, typename T3, typename RET>
struct three_arg_t{
  // Three arguments

  template<RET Func(CLASS_T, const T1&, const T2&, const T3&)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* args){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    T1 a1;
    T2 a2;
    T3 a3;
    if (!wrapped_parse_args(a1, a2, a3, args)){
      return nullptr;
    }
    try{
      return build_result(Func(get_cpp_object(self), a1, a2, a3));
    }
    catch(const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError& error){
      return nullptr;
    }
  }
};

template<typename CLASS_T, typename T1, typename T2, typename T3>
struct three_arg_t<CLASS_T, T1, T2, T3, void>{
  // Three arguments, void specialization

  template<void Func(CLASS_T, const T1&, const T2&, const T3&)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* args){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    T1 a1;
    T2 a2;
    T3 a3;
    if (!wrapped_parse_args(a1, a2, a3, args)){
      return nullptr;
    }
    try{
      Func(get_cpp_object(self), a1, a2, a3);
      return Py_BuildValue("");
    }
    catch(const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename CLASS_T, typename T1, typename T2, typename T3, typename T4, typename RET>
struct four_arg_t{
  // Four arguments

  template<RET Func(CLASS_T, const T1&, const T2&, const T3&, const T4&)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* args){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    T1 a1;
    T2 a2;
    T3 a3;
    T4 a4;
    if (!wrapped_parse_args(a1, a2, a3, a4, args)){
      return nullptr;
    }
    try{
      return build_result(Func(get_cpp_object(self), a1, a2, a3, a4));
    }
    catch(const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError& error){
      return nullptr;
    }
  }
};

template<typename CLASS_T, typename T1, typename T2, typename T3, typename T4>
struct four_arg_t<CLASS_T, T1, T2, T3, T4, void>{
  // Four arguments, void specialization

  template<void Func(CLASS_T, const T1&, const T2&, const T3&, const T4&)>
  static PyObject* call_func(PyObject* rawSelf, PyObject* args){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    T1 a1;
    T2 a2;
    T3 a3;
    T4 a4;
    if (!wrapped_parse_args(a1, a2, a3, a4, args)){
      return nullptr;
    }
    try{
      Func(get_cpp_object(self), a1, a2, a3, a4);
      return Py_BuildValue("");
    }
    catch(const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename RET>
struct free_zero_arg_t{
  // Zero arguments, free function
  template<RET Func()>
  static PyObject* call_func(PyObject*, PyObject* /*args*/){
    try{
      return build_result(Func());
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<>
struct free_zero_arg_t<void>{
  // Zero arguments, free function, void specialization

  template<void Func()>
  static PyObject* call_func(PyObject*, PyObject* /*args*/){
    try{
      Func();
      return Py_BuildValue("");
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename T1, typename RET>
struct free_one_arg_t{
  // One argument, free function

  template<RET Func(const T1&)>
  static PyObject* call_func(PyObject*, PyObject* args){
    T1 a1;
    if (!wrapped_parse_args(a1, args)){
      return nullptr;
    }
    try{
      return build_result(Func(a1));
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename T1>
struct free_one_arg_t<T1, void>{
  // One argument, free function, void-return specialization

  template<void Func(const T1&)>
  static PyObject* call_func(PyObject*, PyObject* args){
    T1 a1;
    if (!wrapped_parse_args(a1, args)){
      return nullptr;
    }
    try{
      Func(a1);
      return Py_BuildValue("");
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename CLASS_T, typename RET>
struct getter_t{
  template<RET Func(CLASS_T)>
  static PyObject* call_func(PyObject* rawSelf, void*){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return nullptr;
    }
    try{
      return build_result(Func(get_cpp_object(self)));
    }
    catch (const PythonError& error){
      return set_error(error);
    }
    catch (const PresetFunctionError&){
      return nullptr;
    }
  }
};

template<typename CLASS_T, typename T1>
struct setter_t{
  // Setter
  template<void Func(CLASS_T, const T1&)>
  static int call_func(PyObject* rawSelf, PyObject* arg, void*){
    GET_TYPED_SELF(CLASS_T, rawSelf);
    if (!faint_side_ok(self)){
      show_error(self);
      return setter_fail;
    }

    try{
      T1 a1;
      int n = 0;
      const int len = PySequence_Check(arg) ? PySequence_Length(arg) : 1;
      if (!parse_flat(a1, arg, n, len)){
        return setter_fail;
      }
      Func(get_cpp_object(self), a1);
      return setter_ok;
    }
    catch (const PythonError& error){
      set_error(error);
      return setter_fail;
    }
    catch (const PresetFunctionError&){
      return setter_fail;
    }
  }
};

// -----------------------------------------------------------------------
// Declarations for resolving the forwarding structure
// -----------------------------------------------------------------------
// Declarations which are used for decltype to resolve the arguments
// of the function pointer passed as argument follow.
//
// Note that they're not defined anywhere -- they're only used to map
// function pointers with typed arguments, e.g.
// > void put_pixel(Canvas&, const IntPoint&)
// to the structures above (e.g one_arg_t) which have a regular Python
// signature wrapper member function, call_func, which produces the typed
// arguments by parsing the Python arguments (PyObject*) and then calls
// the real function.
template<typename CLASS_T>
regular_t<CLASS_T> resolve(PyObject*(*func)(CLASS_T, PyObject*));

template<typename CLASS_T>
regular_void_t<CLASS_T> resolve(void(*func)(CLASS_T, PyObject*));

template<typename CLASS_T, typename RET>
zero_arg_t<CLASS_T, RET> resolve(RET(*func)(CLASS_T));

template<typename CLASS_T, typename T1, typename RET>
one_arg_t<CLASS_T, T1, RET> resolve(RET(*func)(CLASS_T, const T1&));

template<typename CLASS_T, typename T1, typename T2, typename RET>
two_arg_t<CLASS_T, T1, T2, RET> resolve(RET(*func)(CLASS_T, const T1&, const T2&));

template<typename CLASS_T, typename T1, typename T2, typename T3, typename RET>
three_arg_t<CLASS_T, T1, T2, T3, RET> resolve(RET(*func)(CLASS_T, const T1&, const T2&, const T3&));

template<typename CLASS_T, typename T1, typename T2, typename T3, typename T4, typename RET>
four_arg_t<CLASS_T, T1, T2, T3, T4, RET> resolve(RET(*func)(CLASS_T, const T1&, const T2&, const T3&, const T4&));

template<typename RET>
free_zero_arg_t<RET> free_resolve(RET(*func)());

template<typename T1, typename RET>
free_one_arg_t<T1, RET> free_resolve(RET(*func)(const T1&));

template<typename CLASS_T, typename RET>
getter_t<CLASS_T, RET> get_resolve(RET(*func)(CLASS_T));

template<typename CLASS_T, typename T1>
setter_t<CLASS_T, T1> set_resolve(void(*func)(CLASS_T, const T1&));

// Presumably necessary wrapping since adding scope resolution after
// decltype(...) didn't compile
template<typename ForwardingStruct>
struct route{
  typedef ForwardingStruct T;
};

// -----------------------------------------------------------------------
// The macro for use in PyMethodDef
// -----------------------------------------------------------------------
// Macro for use in a PyMethodDef array. Wraps a strongly typed C++-function
// in the guise of a PyObject*(PyObject*, PyObject*).
//
// The C++ function will only be called if the arguments can be parsed
// into the required types (see py-parse.hh).
//
// Explanation:
//  1. Accepts a C++-function (the func argument)
//
//  2. uses the "resolve"-overloads to resolve a
//     template-instantiation of the forwarding structure templates
//
//  3. nests the resolved struct in "route", since decltype(...)::call_func
//     wouldn't compile
//
//  4. specializes call_func with the function pointer so that the
//     function can be called (the resolve-step only gave the signature,
//     not the address).
#define FORWARDER(func)route<decltype(resolve(func))>::T::call_func<func>

// For free functions (with no "self"-object)
#define FREE_FORWARDER(func)route<decltype(free_resolve(func))>::T::call_func<func>

// For getters
#define GET_FORWARDER(func)route<decltype(get_resolve(func))>::T::call_func<func>
#define SET_FORWARDER(func)route<decltype(set_resolve(func))>::T::call_func<func>

} // namespace
#endif
