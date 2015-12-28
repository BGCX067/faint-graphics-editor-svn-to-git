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

#ifndef FAINT_KEYCODE_HH
#define FAINT_KEYCODE_HH
#include "text/utf8-string.hh"

namespace faint{

class Key{
  // A keycode without modifiers.
public:
  explicit Key(int);
  operator int() const;
private:
  int m_key;
};

namespace key{
  extern const Key one;
  extern const Key alt;
  extern const Key asterisk;
  extern const Key A;
  extern const Key B;
  extern const Key C;
  extern const Key H;
  extern const Key V;
  extern const Key E;
  extern const Key D;
  extern const Key I;
  extern const Key J;
  extern const Key O;
  extern const Key P;
  extern const Key Q;
  extern const Key W;
  extern const Key back;
  extern const Key ctrl;
  extern const Key del;
  extern const Key down;
  extern const Key end;
  extern const Key enter;
  extern const Key esc;
  extern const Key home;
  extern const Key left;
  extern const Key minus;
  extern const Key nine;
  extern const Key num_minus;
  extern const Key paragraph;
  extern const Key num_plus;
  extern const Key plus;
  extern const Key pgdn;
  extern const Key pgup;
  extern const Key right;
  extern const Key space;
  extern const Key shift;
  extern const Key up;
  extern const Key zero;
  extern const Key F1;
  extern const Key F2;
  extern const Key F3;
  extern const Key F4;
  extern const Key F5;
  extern const Key F6;
  extern const Key F7;
  extern const Key F8;
  extern const Key F9;
  extern const Key F10;
  extern const Key F11;
  extern const Key F12;
}

class Mod{
  // A set of key modifiers (Alt, Ctrl, Shift or None).
public:
  // Note: Minimize use of Create - prefer using the predefined
  // constants and combinations with +, e.g. Ctrl+Shift
  static Mod Create(int);
  Mod();
  bool Alt() const;
  bool Ctrl() const;
  bool Shift() const;
  bool None() const;
  // Returns this modifier if true, otherwise empty
  Mod If(bool) const;
  Mod operator+(const Mod&) const;
  void operator+=(const Mod&);
  bool operator==(const Mod&) const;
  bool operator<(const Mod&) const;

  // Raw() Should be used only for serialization,
  int Raw() const;
private:
  explicit Mod(int modifier);
  int m_modifiers;
};

extern const Mod Alt;
extern const Mod Ctrl;
extern const Mod Shift;
extern const Mod None;

class KeyPress{
  // A keycode and modifiers.
public:
  KeyPress();
  KeyPress(const Mod&, const Key&);
  KeyPress(const Key&);
  bool Alt() const;
  bool Ctrl() const;
  Key GetKeyCode() const;
  bool HasModifier() const;
  bool Is(const Key&) const; // Ignores modifiers
  bool Is(const Mod&, const Key&) const;
  Mod Modifiers() const;
  utf8_string Name() const;
  bool Shift() const;
  bool operator==(const KeyPress&) const;
  bool operator<(const KeyPress&) const;
private:
  Key m_keyCode;
  Mod m_modifiers;
};

namespace key{
bool affects_alphanumeric_entry(const KeyPress&);
bool affects_numeric_entry(const KeyPress&);
bool command_char(const KeyPress&);
bool numeric(const KeyPress&);
bool modifier(int key);
}

KeyPress operator+(const Mod&, const Key&);

} // namespace

#endif
