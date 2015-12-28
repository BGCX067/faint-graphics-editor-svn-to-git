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
#include "geo/color.hh"
#include "tools/toolid.hh"
#include "util/settingid.hh"

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
namespace faint{
  void send_control_resized_event(wxEvtHandler*);
}
// Event types for ColorEvent
DECLARE_EVENT_TYPE(FAINT_ADD_COLOR, -1)
DECLARE_EVENT_TYPE(FAINT_COPY_COLOR_HEX, -1)
DECLARE_EVENT_TYPE(FAINT_COPY_COLOR_RGB, -1)

class ColorEvent : public wxCommandEvent{
public:
  ColorEvent(wxEventType, const faint::Color&);
  wxEvent* Clone() const;
  faint::Color GetColor() const;
private:
  faint::Color m_color;
};

typedef void (wxEvtHandler::*ColorEvtFunc)(ColorEvent&);

#define ColorEventHandler(func) \
  (wxObjectEventFunction)(wxEventFunction)(wxCommandEventFunction)\
  wxStaticCastEvent(ColorEvtFunc,&func)

DECLARE_EVENT_TYPE(FAINT_CHANGE_TOOL,-1)
class ToolChangeEvent : public wxCommandEvent{
public:
  ToolChangeEvent( ToolId );
  wxEvent* Clone() const;
  ToolId GetTool() const;
private:
  ToolId m_toolId;
};

typedef void (wxEvtHandler::*ToolChangeEvtFunc)(ToolChangeEvent&);
#define ToolChangeEventHandler(func) \
  (wxObjectEventFunction)(wxEventFunction)(wxCommandEventFunction)\
  wxStaticCastEvent(ToolChangeEvtFunc,&func)

// Layer change
DECLARE_EVENT_TYPE(FAINT_CHANGE_LAYER,-1)
class LayerChangeEvent : public wxCommandEvent{
public:
  LayerChangeEvent( Layer::type );
  wxEvent* Clone() const;
  Layer::type GetLayer() const;
private:
  Layer::type m_layer;
};

typedef void (wxEvtHandler::*LayerChangeEvtFunc)(LayerChangeEvent&);

#define LayerChangeEvtHandler(func) \
  (wxObjectEventFunction)(wxEventFunction)(wxCommandEventFunction)\
  wxStaticCastEvent(LayerChangeEvtFunc,&func)

#endif
