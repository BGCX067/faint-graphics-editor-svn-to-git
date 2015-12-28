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

#ifndef FAINT_DUAL_SLIDER_HH
#define FAINT_DUAL_SLIDER_HH
#include "wx/event.h" // Fixme: Consider
#include "geo/range.hh"
#include "gui/slider.hh"

class DualSlider : public wxPanel{
public:
  DualSlider( wxWindow* parent, const ClosedIntRange&, const Interval& initial, const SliderBackground& );
  ~DualSlider();
  Interval GetSelectedInterval() const;
private:
  void OnCaptureLost( wxMouseCaptureLostEvent& );
  void OnEraseBackground( wxEraseEvent& );
  void OnLeftDown( wxMouseEvent& );
  void OnLeftUp( wxMouseEvent& );
  void OnMotion( wxMouseEvent& );
  void OnPaint( wxPaintEvent& );
  void OnSize( wxSizeEvent& );
  void PrepareBitmap();
  void SendSliderChangeEvent();
  void UpdateFromAnchor( int );
  void UpdateFromPos( double, int );
  int WhichHandle( int x );
  int m_anchor; // For moving the entire area
  double m_anchorV1;
  double m_anchorV2;
  SliderBackground* m_background;
  wxBitmap m_bitmap;
  ClosedIntRange m_range;
  double m_v1;
  double m_v2;
  int m_which;

  DECLARE_EVENT_TABLE()
};

DECLARE_EVENT_TYPE(FAINT_DUAL_SLIDER_CHANGE, -1)
class DualSliderEvent : public wxCommandEvent{
public:
  DualSliderEvent(wxEventType, const Interval& );
  wxEvent* Clone() const;
  Interval GetSelectedInterval() const;
private:
  Interval m_interval;
};

typedef void (wxEvtHandler::*DualSliderEvtFunc)(DualSliderEvent&);
#define DualSliderEventHandler(func) \
  (wxObjectEventFunction)(wxEventFunction)(wxCommandEventFunction)	\
  wxStaticCastEvent(DualSliderEvtFunc,&func)

#endif
