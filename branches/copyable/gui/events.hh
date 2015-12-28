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

#ifndef FAINT_EVENTS_HH
#define FAINT_EVENTS_HH
#include "wx/event.h"
#include "tools/tool-id.hh"
#include "util/color.hh"
#include "util/file-path.hh"
#include "util/setting-id.hh"

namespace faint{

// Zoom command events declarations
DECLARE_EVENT_TYPE(ID_ZOOM_IN, -1)
DECLARE_EVENT_TYPE(ID_ZOOM_IN_ALL, -1)
DECLARE_EVENT_TYPE(ID_ZOOM_OUT, -1)
DECLARE_EVENT_TYPE(ID_ZOOM_OUT_ALL, -1)
DECLARE_EVENT_TYPE(ID_ZOOM_100, -1)
DECLARE_EVENT_TYPE(ID_ZOOM_100_ALL, -1)
DECLARE_EVENT_TYPE(ID_ZOOM_FIT, -1)
DECLARE_EVENT_TYPE(ID_ZOOM_FIT_ALL, -1)

// Event for when a control changes size,
// so that parent-panels can re-Layout
DECLARE_EVENT_TYPE(ID_CONTROL_RESIZED, -1)
void send_control_resized_event(wxEvtHandler*);


// PaintEvent
class PaintEvent : public wxCommandEvent{
public:
  PaintEvent(wxEventType, const Paint&);
  wxEvent* Clone() const override;
  Paint GetPaint() const;
private:
  Paint m_src;
};

extern const wxEventType FAINT_ADD_TO_PALETTE;
extern const wxEventTypeTag<PaintEvent> EVT_FAINT_ADD_TO_PALETTE;


// ColorEvent
class ColorEvent : public wxCommandEvent{
public:
  ColorEvent(wxEventType, const Color&);
  wxEvent* Clone() const override;
  Color GetColor() const;
private:
  Color m_color;
};

extern const wxEventType FAINT_COPY_COLOR_HEX;
extern const wxEventType FAINT_COPY_COLOR_RGB;
extern const wxEventTypeTag<ColorEvent> EVT_FAINT_COPY_COLOR_HEX;
extern const wxEventTypeTag<ColorEvent> EVT_FAINT_COPY_COLOR_RGB;


// ToolChangeEvent
class ToolChangeEvent : public wxCommandEvent{
public:
  ToolChangeEvent(ToolId);
  wxEvent* Clone() const override;
  ToolId GetTool() const;
private:
  ToolId m_toolId;
};
extern const wxEventType FAINT_TOOL_CHANGE;
extern const wxEventTypeTag<ToolChangeEvent> EVT_FAINT_TOOL_CHANGE;


// LayerChangeEvent
class LayerChangeEvent : public wxCommandEvent{
public:
  LayerChangeEvent(Layer);
  wxEvent* Clone() const override;
  Layer GetLayer() const;
private:
  Layer m_layer;
};

extern const wxEventType FAINT_LAYER_CHANGE;
extern const wxEventTypeTag<LayerChangeEvent> EVT_FAINT_LAYER_CHANGE;

// OpenFilesEvent
class OpenFilesEvent : public wxCommandEvent{
public:
  OpenFilesEvent(const FileList&);
  wxEvent* Clone() const override;
  const FileList& GetFileNames() const;
private:
  FileList m_files;
};

extern const wxEventType FAINT_OPEN_FILES;
extern const wxEventTypeTag<OpenFilesEvent> EVT_FAINT_OPEN_FILES;

} // namespace
#endif
