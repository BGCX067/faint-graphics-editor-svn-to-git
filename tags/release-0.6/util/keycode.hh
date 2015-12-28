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

namespace key{
  typedef int key_t;
  extern key_t back;
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
  extern key_t right;
  extern key_t up;

  typedef int mod_t;
  bool ctrl_held(mod_t);
  bool shift_held(mod_t);
}

#endif
