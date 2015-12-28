#include "wx/defs.h"
#include <set>
#include <sstream>
#include "util/keycode.hh"

namespace key{
  key_t alt = WXK_ALT;
  key_t apostrophe = 39;
  key_t back = WXK_BACK;
  key_t ctrl = WXK_CONTROL;
  key_t ctrl_b = 2; // Fixme: ctrl_* keycodes are a, probably brittle, hack
  key_t ctrl_d = 4;
  key_t ctrl_i = 9;
  key_t ctrl_enter = 10;
  key_t del = 127;
  key_t down = WXK_DOWN;
  key_t end = WXK_END;
  key_t enter = WXK_RETURN;
  key_t esc = WXK_ESCAPE;
  key_t F1 = WXK_F1;
  key_t F2 = WXK_F2;
  key_t F3 = WXK_F3;
  key_t F4 = WXK_F4;
  key_t F5 = WXK_F5;
  key_t F6 = WXK_F6;
  key_t F7 = WXK_F7;
  key_t F8 = WXK_F8;
  key_t F9 = WXK_F9;
  key_t F10 = WXK_F10;
  key_t F11 = WXK_F11;
  key_t F12 = WXK_F12;
  key_t home = WXK_HOME;
  key_t left = WXK_LEFT;
  key_t minus = 45; // FIXME
  key_t nine = 57;
  key_t num_plus = 388;
  key_t num_minus = 390;
  key_t paragraph = 167;
  key_t pgdn = WXK_PAGEDOWN;
  key_t pgup = WXK_PAGEUP;
  key_t plus = 43;
  key_t right = WXK_RIGHT;
  key_t shift = WXK_SHIFT;
  key_t space = 32;
  key_t up = WXK_UP;
  key_t zero = 48;

  static std::set<int> init_excluded_alpha_numeric_entry(){
    std::set<int> codes;
    codes.insert(F1);
    codes.insert(F2);
    codes.insert(F3);
    codes.insert(F4);
    codes.insert(F5);
    codes.insert(F6);
    codes.insert(F7);
    codes.insert(F8);
    codes.insert(F9);
    codes.insert(F10);
    codes.insert(F11);
    codes.insert(F12);
    return codes;
  }

  const std::set<int>& excluded_alpha_numeric_entry(){
    static const std::set<int> codes = init_excluded_alpha_numeric_entry();
    return codes;
  }

  bool affects_alphanumeric_entry( int key, int /* modifiers */ ){
    // Fixme: Consider excluding some modifier combinations too
    const std::set<int>& excluded(excluded_alpha_numeric_entry());
    return excluded.find(key) == excluded.end();
  }

  bool affects_numeric_entry( int key ){
    return command_char(key) || numeric(key);
  }

  bool command_char( int key ){
    return key == back || key == del;
  }

  bool alt_held(mod_t mod){
    return (mod & wxMOD_ALT) != 0;
  }

  bool ctrl_held(mod_t mod){
    return (mod & wxMOD_CONTROL) != 0;
  }

  bool modifier( int key ){
    return key == shift || key == ctrl || key == alt;
  }

  bool numeric( int key ){
    return key >= zero && key <= nine;
  }

  bool shift_held(mod_t mod){
    return (mod & wxMOD_SHIFT) != 0;
  }

  std::string name( int key ){
    if ( ( 97 <= key && key <= 122 ) || ( 65 <= key && key <= 90 ) || ( zero <= key && key <= nine ) ){
      std::string str;
      str += char(key);
      return str;
    }
    if ( key == alt ){
      return "Alt";
    }
    if ( key == apostrophe ){
      return "'";
    }
    if ( key == back ){
      return "Backspace";
    }
    if ( key == ctrl ){
      return "Ctrl";
    }
    if ( key == del ){
      return "Del";
    }
    if ( key == down ){
      return "Down";
    }
    if ( key == end ) {
      return "End";
    }
    if ( key == enter ){
      return "Enter";
    }
    if ( key == esc ){
      return "Esc";
    }
    if ( key == F1 ){
      return "F1";
    }
    if ( key == F2 ){
      return "F2";
    }
    if ( key == F3 ){
      return "F3";
    }
    if ( key == F4 ){
      return "F4";
    }
    if ( key == F5 ){
      return "F5";
    }
    if ( key == F6 ){
      return "F6";
    }
    if ( key == F7){
      return "F7";
    }
    if ( key == F8 ){
      return "F8";
    }
    if ( key == F9 ){
      return "F9";
    }
    if ( key == F10 ){
      return "F10";
    }
    if ( key == F11 ){
      return "F11";
    }
    if ( key == F12 ){
      return "F12";
    }
    if ( key == home ){
      return "Home";
    }
    if ( key == left ){
      return "Left";
    }
    if ( key == minus ){
      return "-";
    }
    if ( key == num_minus ){
      return "-";
    }
    if ( key == num_plus ){
      return "+";
    }
    if ( key == paragraph ){
      return "Paragraph";
    }
    if ( key == pgdn ){
      return "PgDn";
    }
    if ( key == pgup ){
      return "PgUp";
    }
    if ( key == plus ){
      return "+";
    }
    if ( key == right ){
      return "Right";
    }
    if ( key == shift ){
      return "Shift";
    }
    if ( key == space ){
      return "Space";
    }
    if ( key == up ){
      return "Up";
    }
    std::stringstream ss;
    ss << "<" << key << ">";
    return ss.str();
  }

// Fixme: Duplicates Python, shouldn't be here.
static std::string modifier_text( int modifiers ){
  std::string label = "";
  if ( modifiers == 0 ){
    return label;
  }
  if ( (modifiers & 2) == 2 ){
    label += "Shift+";
  }
  if ( (modifiers & 4) == 4 ){
    label += "Alt+";
  }
  if ( (modifiers & 1) == 1 ){
    label += "Shift+";
  }
  return label;
}

std::string as_text(int keycode, int modifiers){
  return modifier_text(modifiers) + name(keycode);
}

} // namespace
