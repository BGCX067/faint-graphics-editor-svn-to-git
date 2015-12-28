// -*- coding: us-ascii-unix -*-
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

#ifndef FAINT_POS_INFO_HH
#define FAINT_POS_INFO_HH
#include "geo/object-handle.hh"
#include "geo/point.hh"
#include "text/utf8-string.hh"
#include "util/common-fwd.hh"
#include "util/either.hh"
#include "util/keycode.hh"
#include "util/setting-id.hh"
#include "util/status-interface.hh"

namespace faint{

enum class Hit{
  NONE = 0,
  INSIDE = 1,
  NEARBY,
  BOUNDARY,
};

enum class HandleType{
  MOVABLE_POINT,
  EXTENSION_POINT
};

enum TABLET_CURSORS{
  TABLET_CURSOR_PUCK = 0,
  TABLET_CURSOR_STYLUS = 1,
  TABLET_CURSOR_ERASER = 2
};

enum class MouseButton{
  LEFT,
  RIGHT,
  NONE
};

MouseButton the_other_one(MouseButton);

class ToolModifiers{
public:
  void SetLeftMouse();
  void SetRightMouse();
  void SetPrimary();
  void SetSecondary();
  bool LeftMouse() const;
  bool RightMouse() const;

  // Gets the featured mouse-button
  faint::MouseButton MouseButton() const;

  // True if the event features the specified button
  bool Feature(faint::MouseButton) const;
  bool Not(faint::MouseButton) const;

 // Normally ctrl
  bool Primary() const;

  // Normally shift
  bool Secondary() const;

  bool OnlyPrimary() const;
  bool OnlySecondary() const;
  bool Both() const;
  bool Either() const;
  bool Neither() const;
private:
  faint::MouseButton m_mouseButton = faint::MouseButton::NONE;
  bool m_primary = false;
  bool m_secondary = false;
};


typedef Either<Handle,std::pair<object_handle_t, HandleType> > EitherHandle;
// Index of object:s clicked handle, or -1 if no object clicked
typedef Optional<EitherHandle> HandleInfo;


class ObjectInfo{
public:
  ObjectInfo(Object* in_object, Hit in_hitStatus, bool in_selected, const HandleInfo& in_handleInfo)
    : object(in_object),
      hitStatus(in_hitStatus),
      selected(in_selected),
      handleInfo(in_handleInfo)
  {}
  Object* object;
  Hit hitStatus;
  bool selected;
  HandleInfo handleInfo;
};

class PosInfo{
public:
  PosInfo(Canvas& in_canvas, StatusInterface& in_status, const ToolModifiers& in_modifiers, const Point& in_pos, int in_tabletCursor, bool in_inSelection, Layer in_layerType, ObjectInfo& objInfo)
    : canvas(in_canvas),
      status(in_status),
      modifiers(in_modifiers),
      pos(in_pos),
      tabletCursor(in_tabletCursor),
      inSelection(in_inSelection),
      layerType(in_layerType),
      hitStatus(objInfo.hitStatus),
      objSelected(objInfo.selected),
      object(objInfo.object),
      handle(objInfo.handleInfo)
  {}
  Canvas& canvas;
  StatusInterface& status;

  // Modifiers that were active when the event occured
  ToolModifiers modifiers;

  // The mouse position in image coordinates when the event occured
  Point pos;

  // A cursor id from a pen tablet
  int tabletCursor;

  // True if position is inside a raster selection
  bool inSelection;

  // The current layer-type choice
  Layer layerType;

  // Hitstatus of objects, relevant only if object layer
  Hit hitStatus;

  // True if object of hitStatus is a selected object.
  // Relevant only if object layer and hitStatus != HIT_NONE
  bool objSelected;

  // Hovered object (if any)
  Object* object;

  // Index of object:s clicked handle, or -1 if no object clicked
  HandleInfo handle;

  PosInfo& operator=(const PosInfo&) = delete;
};

class KeyInfo{
public:
  KeyInfo(Canvas& in_canvas, StatusInterface& in_status, Layer in_layerType, utf8_char in_ch, const KeyPress& in_key) :
    canvas(in_canvas),
    status(in_status),
    layerType(in_layerType),
    ch(in_ch),
    key(in_key)
  {}
  Canvas& canvas;
  StatusInterface& status;
  Layer layerType;
  utf8_char ch; // Fixme: Make retrievable from KeyPress
  KeyPress key;

  KeyInfo& operator=(const KeyInfo&) = delete;
private:
  KeyInfo();
};

} // namespace

#endif
