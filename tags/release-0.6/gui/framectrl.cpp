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

#include "wx/wx.h"
#include "app/getappcontext.hh"
#include "commands/addframecommand.hh" // Fixme
#include "commands/remove-frame.hh"
#include "commands/reorder-frame.hh"
#include "gui/cursors.hh"
#include "gui/events.hh"
#include "gui/framectrl.hh"
#include "util/artcontainer.hh"
#include "util/convertwx.hh"
#include "util/image.hh" // Fixme
#include "util/util.hh"

struct FrameDragInfo{
  FrameDragInfo()
    : active(false),
      copy(false),
      dragStart(0,0),
      dropFrame(0),
      sourceFrame(0)
  {}

  FrameDragInfo(wxPoint in_dragStart, size_t in_sourceFrame)
    : active(false),
      copy(false),
      dragStart(in_dragStart),
      dropFrame(0),
      sourceFrame(in_sourceFrame)
  {}
  bool active;
  bool copy;
  wxPoint dragStart;
  size_t sourceFrame;
  size_t dropFrame;
};

class FrameListCtrl : public wxPanel {
public:
  FrameListCtrl( wxWindow* parent )
    : wxPanel(parent),
      m_numFrames(0)
  {
    #ifdef __WXMSW__
    SetBackgroundStyle( wxBG_STYLE_PAINT );
    #endif
    SetInitialSize(wxSize(40,40));
  }

  void CreateBitmap(){
    m_bitmap = wxBitmap(wxSize(40 * m_numFrames + 10, 40));
    wxMemoryDC dc(m_bitmap);
    dc.SetBackground(GetBackgroundColour());
    dc.Clear();
    size_t selected = GetAppContext().GetActiveCanvas().GetSelectedFrame();
    wxPen activePen(wxColour(0,0,0));
    wxPen inactivePen(wxColour(0,0,0));
    wxBrush activeBrush(wxColour(255,255,255));
    wxBrush inactiveBrush(wxColour(128,128,128));

    for ( size_t i = 0; i != m_numFrames; i++ ){
      if ( i == selected ){
	dc.SetPen(activePen);
	dc.SetBrush(activeBrush);
      }
      else {
	dc.SetPen(inactivePen);
	dc.SetBrush(inactiveBrush);
      }
      wxPoint pos( i*40 + 5, 0 );
      dc.DrawRectangle( pos, wxSize(35,35) );
    }
    if ( m_dragInfo.active ){
      wxPen dragPen(wxColour(0,0,0), 3, wxSOLID);
      dragPen.SetCap(wxCAP_BUTT);
      dc.SetPen(dragPen);
      dc.DrawLine( m_dragInfo.dropFrame * 40 + 5 - 3, 0, m_dragInfo.dropFrame * 40 + 5 - 3, 35 );
    }
  }

  void SetNumFrames( size_t numFrames ){
    SetInitialSize(wxSize(40 * numFrames + 10,40));
    m_numFrames = numFrames;
    Refresh();
    faint::send_control_resized_event(GetEventHandler());
  }

  void OnLeftDown(wxMouseEvent& event){
    wxPoint pos = event.GetPosition();
    CanvasInterface& active = GetAppContext().GetActiveCanvas();
    size_t frame = GetFrameIndex(pos, active, false);
    assert( frame < active.GetNumFrames());
    active.SelectFrame(frame);
    m_dragInfo = FrameDragInfo(pos, frame);
    CaptureMouse();
    Refresh();
  }

  void OnLeftUp(wxMouseEvent& event){
    if ( HasCapture() ){
      ReleaseMouse();
      SetCursor( wxCURSOR_ARROW );
      if ( m_dragInfo.active ){
	bool ctrlHeld = wxGetKeyState( WXK_CONTROL );
	m_dragInfo.active = false;
	CanvasInterface& canvas = GetAppContext().GetActiveCanvas();
	const faint::ImageList& images(canvas.GetFrames());
	// Fixme: Use events instead of commands
	if ( ctrlHeld ){
	  canvas.RunCommand(new AddFrameCommand(new faint::Image(images.GetImage(m_dragInfo.sourceFrame)), m_dragInfo.dropFrame) );
	}
	else {
	  canvas.RunCommand(reorder_frame_command(m_dragInfo.dropFrame, m_dragInfo.sourceFrame));
	}
	canvas.SelectFrame(std::min(m_dragInfo.dropFrame, canvas.GetNumFrames() - 1));
	Refresh();
      }
    }
  }

  void OnRightDown( wxMouseEvent& event ){
    bool ctrlHeld = wxGetKeyState( WXK_CONTROL );
    if ( !ctrlHeld ){
      return;
    }
    wxPoint pos = event.GetPosition();
    CanvasInterface& active = GetAppContext().GetActiveCanvas();
    size_t index = GetFrameIndex(pos, active, false);
    active.RunCommand(remove_frame_command(index));

  }

  void OnMotion( wxMouseEvent& event ){
    if ( HasCapture() ){
      bool ctrlHeld = wxGetKeyState( WXK_CONTROL );
      if ( m_dragInfo.active ){
	if ( ctrlHeld && ! m_dragInfo.copy ){
	  m_dragInfo.copy = true;
	  SetCursor(to_wx_cursor(Cursor::DRAG_COPY_FRAME));
	}

	size_t frame = GetFrameIndex( event.GetPosition(), GetAppContext().GetActiveCanvas(), true );
	if ( m_dragInfo.dropFrame != frame ){
	  m_dragInfo.dropFrame = frame;
	  Refresh();
	}
      }
      else if ( distance( to_faint(event.GetPosition()), to_faint(m_dragInfo.dragStart) )  > 5 ){
	m_dragInfo.active = true;
	if ( ctrlHeld ){
	  SetCursor( to_wx_cursor(Cursor::DRAG_COPY_FRAME) );
	}
	SetCursor( to_wx_cursor(Cursor::DRAG_FRAME));
      }
    }
  }

  void OnPaint(wxPaintEvent&){
    CreateBitmap();
    size_t selected = GetAppContext().GetActiveCanvas().GetSelectedFrame();
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bitmap, 0, 0);
  }
private:
  size_t GetFrameIndex( const wxPoint& pos, const CanvasInterface& active, bool allowNext ){
    size_t numFrames = active.GetNumFrames();
    size_t frame = pos.x / 40;
    if ( frame < numFrames || ( frame == numFrames && allowNext ) ){
      return frame;
    }
    else {
      return numFrames -1;
    }
  }
  size_t m_numFrames;
  size_t m_dragFrame;
  FrameDragInfo m_dragInfo;
  wxBitmap m_bitmap;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(FrameListCtrl, wxPanel)
EVT_LEFT_DOWN( FrameListCtrl::OnLeftDown )
EVT_LEFT_UP( FrameListCtrl::OnLeftUp )
EVT_MOTION( FrameListCtrl::OnMotion )
EVT_PAINT( FrameListCtrl::OnPaint )
EVT_RIGHT_DOWN( FrameListCtrl::OnRightDown )
END_EVENT_TABLE()

class FrameControlImpl : public wxPanel {
public:
  FrameControlImpl(wxWindow* parent, const ArtContainer& art)
    : wxPanel(parent)
  {
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    // wxWANTS_CHARS prevents sound on keypress when key has focus
    m_addFrameButton = new wxButton(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS|wxBU_EXACTFIT);
    m_addFrameButton->SetBitmap(art.Get("add_frame"));
    sizer->Add(m_addFrameButton);
    m_listCtrl = new FrameListCtrl(this);
    sizer->Add(m_listCtrl);
    SetSizer(sizer);
    Layout();

    // Handle resizing child controls (e.g. color added to palette
    // or grid-control expanded).
    GetEventHandler()->Connect(wxID_ANY,
      ID_CONTROL_RESIZED,
      wxCommandEventHandler(FrameControlImpl::OnChildResized),
      NULL,
      this);

    GetEventHandler()->Connect(wxID_ANY,
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(FrameControlImpl::OnButton),
      NULL,
      this);


    // Fixme: (Out of battery): Connect event handler for layout! (See colorpanel)
  }
  void OnChildResized(wxCommandEvent& event){
    Layout();
    event.Skip();
  }

  void OnButton(wxCommandEvent&){
    CanvasInterface& canvas(GetAppContext().GetActiveCanvas());
    canvas.RunCommand(new AddFrameCommand(canvas.GetSize()));
  }

  void SetNumFrames(size_t numFrames){
    m_listCtrl->SetNumFrames(numFrames);
    assert(numFrames != 0);
    if ( numFrames == 1 ){
      m_listCtrl->Hide();
    }
    else{
      m_listCtrl->Show();
    }

  }

  FrameListCtrl* m_listCtrl;
  wxButton* m_addFrameButton;
};

FrameControl::FrameControl(wxWindow* parent, const ArtContainer& art){
  m_impl = new FrameControlImpl(parent, art);
}

FrameControl::~FrameControl(){
  m_impl = 0; // Deletion is handled by wxWidgets
}

wxWindow* FrameControl::AsWindow(){
  return m_impl;
}

void FrameControl::SetNumFrames( size_t numFrames ){
  m_impl->SetNumFrames(numFrames);
}
