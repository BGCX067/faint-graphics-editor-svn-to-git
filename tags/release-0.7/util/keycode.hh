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

#ifndef FAINT_KEYCODE_HH
#define FAINT_KEYCODE_HH
#include <string>

namespace key{
  typedef int key_t;
  extern key_t one;
  extern key_t alt;
  extern key_t back;
  extern key_t ctrl;
  extern key_t ctrl_b;
  extern key_t ctrl_d;
  extern key_t ctrl_i;
  extern key_t ctrl_enter;
  extern key_t del;
  extern key_t down;
  extern key_t end;
  extern key_t enter;
  extern key_t esc;
  extern key_t home;
  extern key_t left;
  extern key_t minus;
  extern key_t nine;
  extern key_t num_minus;
  extern key_t paragraph;
  extern key_t num_plus;
  extern key_t plus;
  extern key_t pgdn;
  extern key_t pgup;
  extern key_t right;
  extern key_t space;
  extern key_t shift;
  extern key_t up;
  extern key_t zero;
  typedef int mod_t;
  bool affects_numeric_entry( int key );
  bool alt_held(mod_t);
  bool command_char( int key );
  bool ctrl_held(mod_t);
  bool numeric( int key );
  bool shift_held(mod_t);
  bool modifier(int key);

  std::string name( int key );
}
#endif
