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

#ifndef FAINT_CURSORPOSINFO_HH
#define FAINT_CURSORPOSINFO_HH
#include "util/char.hh"
#include "util/keycode.hh"
#include "util/settingid.hh" // Fixme: for Layer

struct CursorPositionInfo{
  CanvasInterface* canvas;
  StatusInterface* status;

  // Modifiers that were active when the event occured
  int modifiers;

  // The mouse position when the event occured
  Point pos;

  // True if position is inside a raster selection
  bool inSelection;

  // The current layer-type choice
  Layer::type layerType;

  // Hitstatus of objects, relevant only if object layer
  int hitStatus; // Fixme: use HitInfo enum

  // True if object of hitStatus is a selected object.
  // Relevant only if object layer and hitStatus != HIT_NONE
  bool objSelected;

  // Hovered object (if any)
  Object* object;

  // Index of object:s clicked handle, or -1 if no object clicked
  int handleIndex;
};

struct KeyInfo{
  CanvasInterface* canvas;
  StatusInterface* status;
  Layer::type layerType;
  faint::utf8_char ch;
  key::key_t keyCode;
  key::mod_t keyModifiers;
};

#endif
