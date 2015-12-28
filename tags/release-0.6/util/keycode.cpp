#include "keycode.hh"
#include "wx/defs.h"

namespace key{
  key_t back = WXK_BACK;
  
  key_t ctrl_b = 2; // Fixme: ctrl_* keycodes are a, probably brittle, hack
  key_t ctrl_d = 4;
  key_t ctrl_i = 9;
  key_t ctrl_enter = 10;
  key_t del = WXK_DELETE;
  key_t down = WXK_DOWN;
  key_t end = WXK_END;
  key_t enter = WXK_RETURN;
  key_t esc = WXK_ESCAPE;
  key_t home = WXK_HOME;
  key_t left = WXK_LEFT;
  key_t right = WXK_RIGHT;
  key_t up = WXK_UP;

  bool ctrl_held(mod_t mod){
    return mod && wxMOD_CONTROL != 0;
  }

  bool shift_held(mod_t mod){
    return mod && wxMOD_SHIFT != 0;
  }
}
