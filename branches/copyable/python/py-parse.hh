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

#ifndef FAINT_PY_PARSE_HH
#define FAINT_PY_PARSE_HH
#include "geo/range.hh"
#include "python/py-fwd.hh"
#include "text/formatting.hh" // for str_int
#include "util/common-fwd.hh"

namespace faint{
bool parse_int(PyObject*, int n, int* value);
bool parse_coord(PyObject*, int n, coord* value);
bool parse_bytes(PyObject*, int, std::string* value);
bool parse_flat(Grid&, PyObject*, int& n, int len);
bool parse_flat(bool&, PyObject*, int& n, int len);
bool parse_flat(int&, PyObject*, int& n, int len);
bool parse_flat(index_t&, PyObject*, int& n, int len);
bool parse_flat(Tri&, PyObject*, int& n, int len);
bool parse_flat(coord&, PyObject*, int& n, int len);
bool parse_flat(IntRect&, PyObject*, int& n, int len);
bool parse_flat(utf8_string&, PyObject*, int& n, int len);
bool parse_flat(std::string&, PyObject*, int& n, int len);
bool parse_flat(Settings&, PyObject*, int& n, int len);
bool parse_flat(Bitmap&, PyObject*, int& n, int len);
bool parse_flat(AngleSpan&, PyObject*, int& n, int len);
bool parse_flat(KeyPress& value, PyObject*, int& n, int len);
bool parse_flat(IntSize&, PyObject*, int& n, int len);
bool parse_flat(Color&, PyObject*, int& n, int len);
bool parse_flat(ColorStop&, PyObject*, int& n, int len);
bool parse_flat(Paint&, PyObject*, int& n, int len);
bool parse_flat(IntPoint&, PyObject*, int& n, int len);
bool parse_flat(LineSegment&, PyObject*, int& n, int len);
bool parse_flat(Point&, PyObject*, int& n, int len);
bool parse_flat(ColRGB&, PyObject*, int& n, int len);
bool parse_flat(Rect&, PyObject*, int& n, int len);
bool parse_flat(Tri&, PyObject*, int& n, int len);

template<typename T>
bool parse_flat(std::vector<T>& vec, PyObject* args, int& n, int len){
  if (len - n < 0){
    // Fixme
    throw ValueError("Too few arguments for parsing vector");
  }

  if (PyUnicode_Check(args)){
    throw TypeError("Expected container, got string");
  }

  if (!PySequence_Check(args)){
    throw TypeError("Not a sequence");
  }

  std::vector<T> temp;
  for (int i = n; i < len; i++){
    T item;
    ScopedRef ref(PySequence_GetItem(args, i));
    if (*ref == nullptr){
      throw ValueError("Failed getting item");
    }
    int n2 = 0;
    if (!parse_flat(item, *ref, n2, 1)){
      throw ValueError("Failed parsing item");
    }
    temp.push_back(item);
  }
  n += resigned(temp.size());
  vec = temp;
  return true;
}

template<int MIN_BOUND, int MAX_BOUND>
bool parse_flat(StaticBoundedInt<MIN_BOUND, MAX_BOUND>& value, PyObject* args, int& n, int len){
  if (len - n < 1){
    throw ValueError("Too few arguments for parsing integer"); // Fixme
  }

  typedef StaticBoundedInt<MIN_BOUND, MAX_BOUND> value_type;
  int raw;
  if (!parse_int(args, n, &raw)){
    return false;
  }
  if (!value_type::Valid(raw)){
    ClosedIntRange range(as_closed_range<value_type>());
    throw ValueError(space_sep("Value", str_int(raw),
        "outside valid range", str_range(range)));
  }

  n += 1;
  value = value_type(raw);
  return true;
}

template<int MIN_BOUND, int MAX_BOUND>
bool parse_flat(StaticBoundedInterval<MIN_BOUND, MAX_BOUND>& range, PyObject* args, int& n, int len){

  if (len - n < 2){
    throw ValueError("Too few arguments for parsing range");
  }

  typedef StaticBoundedInterval<MIN_BOUND, MAX_BOUND> def_range_t;

  int low, high;
  if (!parse_int(args, n, &low) ||
    !parse_int(args, n+1, &high)){
    return false;
  }

  n += 2;

  Interval interval(make_interval(low, high));
  if (!def_range_t::Valid(interval)){
    throw ValueError(space_sep("Threshold of", str_interval(interval),
      "outside valid range",
      str_range(as_closed_range<def_range_t>())));
  }
  range = StaticBoundedInterval<MIN_BOUND, MAX_BOUND>(interval);
  return true;
}

template<typename T>
bool unary_item(const T&){
  return false;
}

inline bool unary_item(const Bitmap&){
  return true;
}

inline bool unary_item(const std::string&){
  return true;
}

inline bool unary_item(const int&){
  return true;
}

inline bool unary_item(const coord&){
  return true;
}

inline bool unary_item(const bool&){
  return true;
}

template<typename T>
bool parse_item(T& item, PyObject* args, int& n, int len, bool allowFlat){
  if (len - n < 1){
    throw ValueError("Incorrect number of arguments");
  }

  ScopedRef elems(PySequence_GetItem(args, n));
  if (!PySequence_Check(*elems)){
    if (allowFlat || unary_item(item)){
      return parse_flat(item, args, n, len);
    }
    throw TypeError("Must be a sequence");
  }
  int nOther = 0;
  bool ok = parse_flat(item, *elems, nOther, PySequence_Length(*elems));
  if (!ok){
    return false;
  }
  n += 1;
  return true;
}

template<typename T>
bool parse_option(Optional<T>& item, PyObject* args, int& n, int len, bool allowFlat){
  T temp;
  if (!parse_item(temp, args, n, len, allowFlat)){
    return false;
  }
  item.Set(temp);
  return true;
}

template<typename T1>
bool parse_args(T1& a1, PyObject* args){
  int len = PySequence_Length(args);
  int n = 0;
  if (!parse_item(a1, args, n, len, true)){
    return false;
  }
  // Fixme: Check that all arguments are consumed. My trivial attempt at this
  // didn't work.

  return true;
}

template<typename T1>
bool parse_args(Optional<T1>& a1, PyObject* args){
  int len = PySequence_Length(args);
  if (len == 0){
    return true;
  }

  int n = 0;
  if (!parse_option(a1, args, n, len, true)){
    return false;
  }
  // Fixme: Check that all arguments are consumed. My trivial attempt at this
  // didn't work.

  return true;
}

template<typename T1, typename T2>
bool parse_args(T1& a1, Optional<T2>& a2, PyObject* args){
  int len = PySequence_Length(args);
  int n = 0;
  if (!parse_item(a1, args, n, len, true)){
    return false;
  }
  if (n < len && !parse_option(a2, args, n, len, false)){
    return false;
  }
  return true;
}

template<typename T1, typename T2>
bool parse_args(T1& a1, T2& a2, PyObject* args){
  int len = PySequence_Length(args);
  int n = 0;
  if (!parse_item(a1, args, n, len, false)){
    return false;
  }
  if (!parse_item(a2, args, n, len, false)){
    return false;
  }
  return true;
}

template<typename T1, typename T2, typename T3>
bool parse_args(T1& a1, T2& a2, T3& a3, PyObject* args){
  int len = PySequence_Length(args);
  int n = 0;
  if (!parse_item(a1, args, n, len, false)){
    return false;
  }
  if (!parse_item(a2, args, n, len, false)){
    return false;
  }
  if (!parse_item(a3, args, n, len, false)){
    return false;
  }
  return true;
}

template<typename T1, typename T2, typename T3>
bool parse_args(T1& a1, Optional<T2>& a2, Optional<T3>& a3, PyObject* args){
  int len = PySequence_Length(args);
  int n = 0;
  if (!parse_item(a1, args, n, len, false)){
    return false;
  }
  if (n < len && !parse_option(a2, args, n, len, false)){
    return false;
  }
  if (n < len && !parse_option(a3, args, n, len, false)){
    return false;
  }
  return true;
}

template<typename T1, typename T2, typename T3, typename T4>
bool parse_args(T1& a1, T2& a2, T3& a3, T4& a4, PyObject* args){
  int len = PySequence_Length(args);
  int n = 0;
  if (!parse_item(a1, args, n, len, true)){
    return false;
  }
  if (!parse_item(a2, args, n, len, true)){
    return false;
  }
  if (!parse_item(a3, args, n, len, true)){
    return false;
  }
  if (!parse_item(a4, args, n, len, true)){
    return false;
  }
  return true;
}

PyObject* build_result(const Angle&);
PyObject* build_result(bool);
PyObject* build_result(const Color&);
PyObject* build_result(const FilePath&);
PyObject* build_result(const DirPath&);
PyObject* build_result(const utf8_string&);
PyObject* build_result(const BoundObject&);
PyObject* build_result(const ColorStop&);
PyObject* build_result(const Point&);
PyObject* build_result(const index_t&);
PyObject* build_result(const IntPoint&);

inline PyObject* build_result(int value){
  return Py_BuildValue("i", value);
}

inline PyObject* build_result(coord value){
  return Py_BuildValue("d", value);
}

PyObject* build_result(const Paint&);
PyObject* build_result(const Bitmap&);
PyObject* build_result(const IntRect&);
PyObject* build_result(const Rect&);
PyObject* build_result(const IntSize&);

inline PyObject* build_result(PyObject* result){
  return result;
}
PyObject* build_result(const std::string&);
PyObject* build_result(const CanvasGrid&);
PyObject* build_result(const Tri&);
PyObject* build_result(Canvas&);
PyObject* build_result(const Settings&);
PyObject* build_result(const line_t&);

template<typename T1, typename T2>
inline PyObject* build_result(const std::pair<T1, T2>& pair){
  PyObject* first = build_result(pair.first);
  PyObject* second = build_result(pair.second);
  return PyTuple_Pack(2, first, second);
}

template<typename T>
PyObject* build_result(const Optional<T>& opt){
  if (!opt.IsSet()){
    return Py_BuildValue("");
  }
  return build_result(opt.Get());
}

template<typename T>
inline PyObject* build_result(const std::vector<T>& v){
  PyObject* py_list = PyList_New(resigned(v.size()));
  for (size_t i = 0; i!= v.size(); i++){
    PyList_SetItem(py_list, resigned(i), build_result(v[i]));
  }
  return py_list;
}

template<typename T>
int item_length(const T&){
  return 1;
}

inline int item_length(const Color&){
  return 3;
}

}

#endif
