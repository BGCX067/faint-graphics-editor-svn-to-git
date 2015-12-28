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

#ifndef FAINT_SLIDER_HH
#define FAINT_SLIDER_HH
#include "util/commonfwd.hh"
#include "geo/range.hh"
#include "wx/bitmap.h"
#include "wx/panel.h"

class SliderBackground{
public:
  virtual ~SliderBackground(){}
  virtual void Draw( faint::Bitmap&, const IntSize& ) = 0;
  virtual SliderBackground* Clone() const = 0;
};

class SliderRectangleBackground : public SliderBackground{
public:
  void Draw( faint::Bitmap&, const IntSize& ) override;
  SliderBackground* Clone() const override;
};

enum class SliderDir{ HORIZONTAL, VERTICAL };

class Slider : public wxPanel{
public:
  Slider( wxWindow* parent, const ClosedIntRange&, int initial, SliderDir, const SliderBackground& );
  ~Slider();
  int GetValue() const;
  void SetBackground( const SliderBackground& );
  void SetValue( int value );
private:
  void OnCaptureLost( wxMouseCaptureLostEvent& );
  void OnEraseBackground( wxEraseEvent& );
  void OnLeftDown( wxMouseEvent& );
  void OnLeftUp( wxMouseEvent& );
  void OnMotion( wxMouseEvent& );
  void OnPaint( wxPaintEvent& );
  void OnSize( wxSizeEvent& );
  void PrepareBitmap();
  SliderBackground* m_background;
  wxBitmap m_bitmap;
  SliderDir m_dir;
  ClosedIntRange m_range;
  double m_value;
  DECLARE_EVENT_TABLE()
};

DECLARE_EVENT_TYPE(FAINT_SLIDER_CHANGE, -1)
class SliderEvent : public wxCommandEvent{
public:
  SliderEvent(wxEventType, int value );
  wxEvent* Clone() const;
  int GetValue() const;
private:
  int m_value;
};

typedef void (wxEvtHandler::*SliderEvtFunc)(SliderEvent&);
#define SliderEventHandler(func) \
  (wxObjectEventFunction)(wxEventFunction)(wxCommandEventFunction)	\
  wxStaticCastEvent(SliderEvtFunc,&func)


#endif
