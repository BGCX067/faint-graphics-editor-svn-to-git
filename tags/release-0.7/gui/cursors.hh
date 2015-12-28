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

#ifndef FAINT_CURSORS_HH
#define FAINT_CURSORS_HH

struct Cursor{
  enum type{
    ADD_POINT,
    ARROW,
    BLANK, // No cursor
    BRUSH,
    CARET,
    CLONE,
    CROSSHAIR,
    DONT_CARE, // Keep current cursor
    FILL,
    MOVE,
    MOVE_POINT,
    MOVE_SELECTION,
    PEN,
    PICKER,
    RESIZE_NE,
    RESIZE_NS,
    RESIZE_NW,
    RESIZE_WE,
    ROTATE_RIGHT,
    SQUARE_CROSS,
    TEXT_CROSSHAIR,
    DRAG_FRAME,
    DRAG_COPY_FRAME
  };
};

#endif
