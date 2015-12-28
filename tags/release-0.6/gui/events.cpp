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

#include "events.hh"

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
namespace faint{
  void send_control_resized_event(wxEvtHandler* handler){
    wxCommandEvent sizeEvent(ID_CONTROL_RESIZED);
    sizeEvent.SetEventObject(handler);
    handler->ProcessEvent(sizeEvent);
  }
}

// ColorEvent
const wxEventType FAINT_ADD_COLOR = wxNewEventType();
const wxEventType FAINT_COPY_COLOR_HEX = wxNewEventType();
const wxEventType FAINT_COPY_COLOR_RGB = wxNewEventType();

ColorEvent::ColorEvent(wxEventType type, const faint::Color& color)
  : wxCommandEvent( type, -1 ),
    m_color(color)
{}

wxEvent* ColorEvent::Clone() const{
  return new ColorEvent(*this);
}

faint::Color ColorEvent::GetColor() const{
  return m_color;
}

// ToolChangeEvent
const wxEventType FAINT_CHANGE_TOOL = wxNewEventType();
ToolChangeEvent::ToolChangeEvent( ToolId toolId )
  : wxCommandEvent( FAINT_CHANGE_TOOL, -1 ),
    m_toolId(toolId)
{}

wxEvent* ToolChangeEvent::Clone() const{
  return new ToolChangeEvent(*this);
}

ToolId ToolChangeEvent::GetTool() const{
  return m_toolId;
}

// LayerChangeEvent
const wxEventType FAINT_CHANGE_LAYER = wxNewEventType();
LayerChangeEvent::LayerChangeEvent( Layer::type layer )
  : wxCommandEvent( FAINT_CHANGE_LAYER, -1 ),
    m_layer(layer)
{}

wxEvent* LayerChangeEvent::Clone() const{
  return new LayerChangeEvent(*this);
}

Layer::type LayerChangeEvent::GetLayer() const{
  return m_layer;
}
