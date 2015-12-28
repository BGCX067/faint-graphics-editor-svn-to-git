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

#include "gui/events.hh"

namespace faint{

// Zoom command event definitions
DEFINE_EVENT_TYPE(ID_ZOOM_IN)
DEFINE_EVENT_TYPE(ID_ZOOM_IN_ALL)
DEFINE_EVENT_TYPE(ID_ZOOM_OUT)
DEFINE_EVENT_TYPE(ID_ZOOM_OUT_ALL)
DEFINE_EVENT_TYPE(ID_ZOOM_100)
DEFINE_EVENT_TYPE(ID_ZOOM_100_ALL)
DEFINE_EVENT_TYPE(ID_ZOOM_FIT)
DEFINE_EVENT_TYPE(ID_ZOOM_FIT_ALL)

DEFINE_EVENT_TYPE(ID_CONTROL_RESIZED)
void send_control_resized_event(wxEvtHandler* handler){
  wxCommandEvent sizeEvent(ID_CONTROL_RESIZED);
  sizeEvent.SetEventObject(handler);
  handler->ProcessEvent(sizeEvent);
}

// PaintEvent
const wxEventType FAINT_ADD_TO_PALETTE = wxNewEventType();
const wxEventTypeTag<PaintEvent> EVT_FAINT_ADD_TO_PALETTE(FAINT_ADD_TO_PALETTE);
PaintEvent::PaintEvent(wxEventType type, const Paint& src)
  : wxCommandEvent(type, -1),
    m_src(src)
{}

wxEvent* PaintEvent::Clone() const{
  return new PaintEvent(*this);
}

Paint PaintEvent::GetPaint() const{
  return m_src;
}

// ColorEvent
const wxEventType FAINT_COPY_COLOR_HEX = wxNewEventType();
const wxEventType FAINT_COPY_COLOR_RGB = wxNewEventType();
const wxEventTypeTag<ColorEvent> EVT_FAINT_COPY_COLOR_HEX(FAINT_COPY_COLOR_HEX);
const wxEventTypeTag<ColorEvent> EVT_FAINT_COPY_COLOR_RGB(FAINT_COPY_COLOR_RGB);

ColorEvent::ColorEvent(wxEventType type, const Color& color)
  : wxCommandEvent(type, -1),
    m_color(color)
{}

wxEvent* ColorEvent::Clone() const{
  return new ColorEvent(*this);
}

Color ColorEvent::GetColor() const{
  return m_color;
}

// ToolChangeEvent
ToolChangeEvent::ToolChangeEvent(ToolId toolId)
  : wxCommandEvent(FAINT_TOOL_CHANGE, -1),
    m_toolId(toolId)
{}

wxEvent* ToolChangeEvent::Clone() const{
  return new ToolChangeEvent(*this);
}

ToolId ToolChangeEvent::GetTool() const{
  return m_toolId;
}

const wxEventType FAINT_TOOL_CHANGE = wxNewEventType();
const wxEventTypeTag<ToolChangeEvent> EVT_FAINT_TOOL_CHANGE(FAINT_TOOL_CHANGE);


// LayerChangeEvent
LayerChangeEvent::LayerChangeEvent(Layer layer)
  : wxCommandEvent(FAINT_LAYER_CHANGE, -1),
    m_layer(layer)
{}

wxEvent* LayerChangeEvent::Clone() const{
  return new LayerChangeEvent(*this);
}

Layer LayerChangeEvent::GetLayer() const{
  return m_layer;
}
const wxEventType FAINT_LAYER_CHANGE = wxNewEventType();
const wxEventTypeTag<LayerChangeEvent> EVT_FAINT_LAYER_CHANGE(FAINT_LAYER_CHANGE);


// OpenFilesEvent
OpenFilesEvent::OpenFilesEvent(const FileList& files)
  : wxCommandEvent(FAINT_OPEN_FILES, -1),
    m_files(files)
{}

wxEvent* OpenFilesEvent::Clone() const{
  return new OpenFilesEvent(*this);
}

const FileList& OpenFilesEvent::GetFileNames() const{
  return m_files;
}

const wxEventType FAINT_OPEN_FILES = wxNewEventType();
const wxEventTypeTag<OpenFilesEvent> EVT_FAINT_OPEN_FILES(FAINT_OPEN_FILES);

} // namespace
