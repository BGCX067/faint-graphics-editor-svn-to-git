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

#include "wx/defs.h"
#include <set>
#include <sstream>
#include "util/convenience.hh"
#include "util/keycode.hh"

namespace faint{

Key::Key(int keyCode)
  : m_key(keyCode)
{}

Key::operator int() const{
  return m_key;
}

namespace key{
  const Key alt(WXK_ALT);
  const Key asterisk(39);
  const Key back(WXK_BACK);
  const Key ctrl(WXK_CONTROL);
  const Key A('A');
  const Key B('B');
  const Key C('C');
  const Key D('D');
  const Key E('E');
  const Key H('H');
  const Key I('I');
  const Key J('J');
  const Key O('O');
  const Key P('P');
  const Key Q('Q');
  const Key V('V');
  const Key W('W');
  const Key del(127);
  const Key down(WXK_DOWN);
  const Key end(WXK_END);
  const Key enter(WXK_RETURN);
  const Key esc(WXK_ESCAPE);
  const Key F1(WXK_F1);
  const Key F2(WXK_F2);
  const Key F3(WXK_F3);
  const Key F4(WXK_F4);
  const Key F5(WXK_F5);
  const Key F6(WXK_F6);
  const Key F7(WXK_F7);
  const Key F8(WXK_F8);
  const Key F9(WXK_F9);
  const Key F10(WXK_F10);
  const Key F11(WXK_F11);
  const Key F12(WXK_F12);
  const Key home(WXK_HOME);
  const Key left(WXK_LEFT);
  const Key minus(45);
  const Key nine(57);
  const Key num_plus(388);
  const Key num_minus(390);
  const Key paragraph(167);
  const Key pgdn(WXK_PAGEDOWN);
  const Key pgup(WXK_PAGEUP);
  const Key plus(43);
  const Key right(WXK_RIGHT);
  const Key shift(WXK_SHIFT);
  const Key space(32);
  const Key up(WXK_UP);
  const Key zero(48);

  static std::set<KeyPress> init_excluded_alpha_numeric_entry(){
    std::set<KeyPress> codes;
    codes.insert(KeyPress(F1));
    codes.insert(KeyPress(F2));
    codes.insert(KeyPress(F3));
    codes.insert(KeyPress(F4));
    codes.insert(KeyPress(F5));
    codes.insert(KeyPress(F6));
    codes.insert(KeyPress(F7));
    codes.insert(KeyPress(F8));
    codes.insert(KeyPress(F9));
    codes.insert(KeyPress(F10));
    codes.insert(KeyPress(F11));
    codes.insert(KeyPress(F12));
    return codes;
  }

  const std::set<KeyPress>& excluded_alpha_numeric_entry(){
    static const std::set<KeyPress> codes = init_excluded_alpha_numeric_entry();
    return codes;
  }

  bool affects_alphanumeric_entry(const KeyPress& key){
    // Fixme: Consider excluding some modifier combinations too
    const std::set<KeyPress>& excluded(excluded_alpha_numeric_entry());
    return excluded.find(key) == excluded.end();
  }

  static bool navigation(const KeyPress& key){
    return key.Is(left) || key.Is(right) || key.Is(up) || key.Is(down) || key.Is(home) || key.Is(end);
  }

  bool affects_numeric_entry(const KeyPress& key){
    return command_char(key) || numeric(key) || navigation(key);
  }

  bool command_char(const KeyPress& key){
    return key.GetKeyCode() == back || key.GetKeyCode() == del;
  }

  bool modifier(int key){
    return key == shift || key == ctrl || key == alt;
  }

  bool numeric(const KeyPress& key){
    return !key.HasModifier() &&
      zero <= key.GetKeyCode() && key.GetKeyCode() <= nine;
  }

  utf8_string name(int key){
    if ((97 <= key && key <= 122) || (65 <= key && key <= 90) || (zero <= key && key <= nine)){
      std::string str;
      str += char(key);
      return utf8_string(str);
    }
    if (key == alt){
      return "Alt";
    }
    if (key == asterisk){
      return "*";
    }
    if (key == back){
      return "Backspace";
    }
    if (key == ctrl){
      return "Ctrl";
    }
    if (key == del){
      return "Del";
    }
    if (key == down){
      return "Down";
    }
    if (key == end) {
      return "End";
    }
    if (key == enter){
      return "Enter";
    }
    if (key == esc){
      return "Esc";
    }
    if (key == F1){
      return "F1";
    }
    if (key == F2){
      return "F2";
    }
    if (key == F3){
      return "F3";
    }
    if (key == F4){
      return "F4";
    }
    if (key == F5){
      return "F5";
    }
    if (key == F6){
      return "F6";
    }
    if (key == F7){
      return "F7";
    }
    if (key == F8){
      return "F8";
    }
    if (key == F9){
      return "F9";
    }
    if (key == F10){
      return "F10";
    }
    if (key == F11){
      return "F11";
    }
    if (key == F12){
      return "F12";
    }
    if (key == home){
      return "Home";
    }
    if (key == left){
      return "Left";
    }
    if (key == minus){
      return "-";
    }
    if (key == num_minus){
      return "-";
    }
    if (key == num_plus){
      return "+";
    }
    if (key == paragraph){
      return "Paragraph";
    }
    if (key == pgdn){
      return "PgDn";
    }
    if (key == pgup){
      return "PgUp";
    }
    if (key == plus){
      return "+";
    }
    if (key == right){
      return "Right";
    }
    if (key == shift){
      return "Shift";
    }
    if (key == space){
      return "Space";
    }
    if (key == up){
      return "Up";
    }
    std::stringstream ss;
    ss << "<" << key << ">";
    return utf8_string(ss.str());
  }

// Fixme: Duplicates Python
  static utf8_string modifier_text(const Mod& modifiers){
  if (modifiers.None()){
    return "";
  }

  std::string label = "";
  if (modifiers.Ctrl()){
    label += "Ctrl+";
  }
  if (modifiers.Shift()){
    label += "Shift+";
  }
  if (modifiers.Alt()){
    label += "Alt+";
  }
  return utf8_string(label);
}

utf8_string as_text(int keycode, const Mod& modifiers){
  return modifier_text(modifiers) + name(keycode);
}

} // namespace

KeyPress::KeyPress()
  : m_keyCode(0),
    m_modifiers(None)
{}

KeyPress::KeyPress(const Mod& modifiers, const Key& keyCode) :
  m_keyCode(keyCode),
  m_modifiers(modifiers)
{}

KeyPress::KeyPress(const Key& keyCode) :
  m_keyCode(keyCode)
{}

bool KeyPress::Alt() const{
  return m_modifiers.Alt();
}

bool KeyPress::Ctrl() const{
  return m_modifiers.Ctrl();
}

Key KeyPress::GetKeyCode() const{
  return m_keyCode;
}

bool KeyPress::HasModifier() const{
  return !m_modifiers.None();
}

bool KeyPress::Is(const Key& keyCode) const{
  return m_keyCode == keyCode;
}

bool KeyPress::Is(const Mod& modifiers, const Key& keyCode) const{
  return m_keyCode == keyCode && m_modifiers == modifiers;
}

Mod KeyPress::Modifiers() const{
  return m_modifiers;
}

utf8_string KeyPress::Name() const{
  return key::as_text(m_keyCode, m_modifiers);
}

bool KeyPress::Shift() const{
  return m_modifiers.Shift();
}

bool KeyPress::operator==(const KeyPress& other) const{
  return m_keyCode == other.m_keyCode &&
    m_modifiers == other.m_modifiers;
}

bool KeyPress::operator<(const KeyPress& other) const{
  return m_keyCode < other.m_keyCode ||
    (m_keyCode == other.m_keyCode &&
      m_modifiers < other.m_modifiers);
}

Mod::Mod()
  : m_modifiers(0)
{}

Mod::Mod(int modifiers)
  : m_modifiers(modifiers)
{}

Mod Mod::Create(int modifiers){
  return Mod(modifiers);
}

Mod Mod::operator+(const Mod& other) const{
  return Mod(m_modifiers | other.m_modifiers);
}

void Mod::operator+=(const Mod& other) {
  m_modifiers |= other.m_modifiers;
}

bool Mod::operator<(const Mod& other) const{
  return m_modifiers < other.m_modifiers;
}

bool Mod::operator==(const Mod& other) const{
  return m_modifiers == other.m_modifiers;
}

bool Mod::Alt() const{
  return fl(wxMOD_ALT, m_modifiers);
}

bool Mod::Ctrl() const{
  return fl(wxMOD_CONTROL, m_modifiers);
}

Mod Mod::If(bool cond) const{
  return cond ? *this : Mod();
}

int Mod::Raw() const{
  return m_modifiers;
}

bool Mod::Shift() const{
  return fl(wxMOD_SHIFT, m_modifiers);
}

bool Mod::None() const{
  return m_modifiers == 0;
}

KeyPress operator+(const Mod& mod, const Key& key){
  return KeyPress(mod, key);
}

const Mod None(Mod::Create(0));
const Mod Alt(Mod::Create(wxMOD_ALT));
const Mod Ctrl(Mod::Create(wxMOD_CONTROL));
const Mod Shift(Mod::Create(wxMOD_SHIFT));

} // namespace
