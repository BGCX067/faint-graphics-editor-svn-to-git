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

#include <sstream>
#include "wx/wx.h"
#include "wx/settings.h"
#include "app/getappcontext.hh"
#include "commands/add-frame-cmd.hh" // Fixme
#include "commands/remove-frame-cmd.hh"
#include "commands/reorder-frame-cmd.hh"
#include "gui/cursors.hh"
#include "gui/events.hh"
#include "gui/framectrl.hh"
#include "util/artcontainer.hh"
#include "util/convertwx.hh"
#include "util/image.hh" // Fixme
#include "util/util.hh"

#ifdef __WXMSW__
#define FRAMECTRL_BORDER_STYLE wxBORDER_THEME
#else
#define FRAMECTRL_BORDER_STYLE wxBORDER_NONE
#endif

struct FrameDragInfo{
  FrameDragInfo()
    : active(false),
      copy(false),
      dragStart(0,0),
      dropFrame(0),
      sourceFrame(0)
  {}

  FrameDragInfo(wxPoint in_dragStart, const index_t& in_sourceFrame)
    : active(false),
      copy(false),
      dragStart(in_dragStart),
      dropFrame(0),
      sourceFrame(in_sourceFrame)
  {}
  bool active;
  bool copy;
  wxPoint dragStart;
  index_t sourceFrame;
  index_t dropFrame;
};

static std::string get_frame_label( const index_t& index ){
  std::stringstream ss;
  ss << "Frame " << index.Get() + 1;
  return ss.str();
}

class FrameListCtrl : public wxPanel {
public:
  FrameListCtrl( wxWindow* parent, const wxBitmap& closeFrame )
    : wxPanel(parent),
      m_numFrames(0),
      m_closeFrameBitmap(closeFrame),
      m_frameBoxSize(35, 38)
  {
    #ifdef __WXMSW__
    SetBackgroundStyle( wxBG_STYLE_PAINT );
    #endif
  }

  void CreateBitmap(){
    // Fixme: Constants
    m_bitmap = wxBitmap(wxSize(40 * m_numFrames.Get() + 10, 40));
    wxMemoryDC dc(m_bitmap);
    dc.SetBackground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    dc.Clear();
    size_t selected = GetAppContext().GetActiveCanvas().GetSelectedFrame().Get();
    wxPen activePen(wxColour(0,0,0));
    wxPen inactivePen(wxColour(0,0,0));
    wxBrush activeBrush(wxColour(255,255,255));
    wxBrush inactiveBrush(wxColour(128,128,128));

    size_t numFrames(m_numFrames.Get());
    for ( size_t i = 0; i != numFrames; i++ ){
      if ( i == selected ){
	dc.SetPen(activePen);
	dc.SetBrush(activeBrush);
      }
      else {
	dc.SetPen(inactivePen);
	dc.SetBrush(inactiveBrush);
      }
      wxPoint pos( i*40 + 5, 0 );
      dc.DrawRectangle( pos, m_frameBoxSize );
      if ( i == selected ){
	dc.DrawBitmap( m_closeFrameBitmap, pos + wxPoint(m_frameBoxSize.GetWidth() - m_closeFrameBitmap.GetWidth() - 1, 1),
	  true ); // Use mask
      }
    }
    if ( m_dragInfo.active ){
      wxPen dragPen(wxColour(0,0,0), 3, wxSOLID);
      dragPen.SetCap(wxCAP_BUTT);
      dc.SetPen(dragPen);
      dc.DrawLine( m_dragInfo.dropFrame.Get() * 40 + 5 - 3, 0, m_dragInfo.dropFrame.Get() * 40 + 5 - 3, 35 ); // Fixme: Use constants
    }
  }

  void SetNumFrames( const index_t& numFrames ){
    SetInitialSize(wxSize(40 * numFrames.Get() + 10,40));
    m_numFrames = numFrames;
    Refresh();
    faint::send_control_resized_event(GetEventHandler());
  }

  void OnLeaveWindow(wxMouseEvent&){
    GetAppContext().GetStatusInfo().SetMainText("");
  }

  void OnLeftDown(wxMouseEvent& event){
    wxPoint pos = event.GetPosition();
    CanvasInterface& active = GetAppContext().GetActiveCanvas();
    index_t frame = GetFrameIndex(pos, active, false);
    size_t selected = active.GetSelectedFrame().Get();
    if ( frame == index_t(selected) && CloseBoxHitTest(pos, frame.Get()) ){
      CanvasInterface& active = GetAppContext().GetActiveCanvas();
      active.RunCommand(remove_frame_command(frame));
      return;
    }

    assert( frame < active.GetNumFrames());
    active.SelectFrame(frame);
    m_dragInfo = FrameDragInfo(pos, frame);
    CaptureMouse();
    Refresh();
  }

  void OnLeftUp(wxMouseEvent&){
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
	  canvas.RunCommand(reorder_frame_command(New(m_dragInfo.dropFrame), Old(m_dragInfo.sourceFrame)));
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
    index_t index = GetFrameIndex(pos, active, false);
    active.RunCommand(remove_frame_command(index));
  }

  void OnMotion( wxMouseEvent& event ){
    if ( !HasCapture() ){
      wxPoint pos = event.GetPosition();
      CanvasInterface& active = GetAppContext().GetActiveCanvas();
      index_t frame = GetFrameIndex(pos, active, false);
      StatusInterface& status = GetAppContext().GetStatusInfo();
      if ( frame == active.GetSelectedFrame() && CloseBoxHitTest(pos, frame.Get()) ){
	status.SetMainText("Click to Remove Frame");
      }
      else {
	status.SetMainText(get_frame_label(frame));
      }
    }
    else {
      bool ctrlHeld = wxGetKeyState( WXK_CONTROL );
      if ( m_dragInfo.active ){
	if ( ctrlHeld && ! m_dragInfo.copy ){
	  m_dragInfo.copy = true;
	  SetCursor(to_wx_cursor(Cursor::DRAG_COPY_FRAME));
	}

	index_t frame = GetFrameIndex( event.GetPosition(), GetAppContext().GetActiveCanvas(), true );
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
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bitmap, 0, 0);
  }
private:
  index_t GetFrameIndex( const wxPoint& pos, const CanvasInterface& active, bool allowNext ){
    index_t numFrames(active.GetNumFrames());
    index_t frame(pos.x / 40);
    if ( frame < numFrames || ( frame == numFrames && allowNext ) ){
      return frame;
    }
    else {
      return numFrames -1;
    }
  }

  bool CloseBoxHitTest( const wxPoint& pos, size_t index ){
    wxRect closeRect(wxPoint(index * 40 + 5 + m_frameBoxSize.GetWidth() - m_closeFrameBitmap.GetWidth() - 1, 0), m_closeFrameBitmap.GetSize());
    return closeRect.Contains(pos);
  }

  index_t m_numFrames;
  FrameDragInfo m_dragInfo;
  wxBitmap m_bitmap;
  wxBitmap m_closeFrameBitmap;
  wxSize m_frameBoxSize;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(FrameListCtrl, wxPanel)
EVT_LEAVE_WINDOW( FrameListCtrl::OnLeaveWindow )
EVT_LEFT_DOWN( FrameListCtrl::OnLeftDown )
EVT_LEFT_DCLICK( FrameListCtrl::OnLeftDown ) // Handle double click as well to avoid lulls in Frame-closing
EVT_LEFT_UP( FrameListCtrl::OnLeftUp )
EVT_MOTION( FrameListCtrl::OnMotion )
EVT_PAINT( FrameListCtrl::OnPaint )
EVT_RIGHT_DOWN( FrameListCtrl::OnRightDown )
END_EVENT_TABLE()

class FrameControlImpl : public wxPanel {
public:
  FrameControlImpl(wxWindow* parent, const ArtContainer& art)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|FRAMECTRL_BORDER_STYLE)
  {
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    // wxWANTS_CHARS prevents sound on keypress when button has focus
    wxButton* addFrameButton = new wxButton(this, wxID_ANY, "", wxDefaultPosition, wxSize(60,50), wxWANTS_CHARS);
    addFrameButton->SetBitmap(art.Get("add_frame"));
    addFrameButton->SetToolTip("Add Frame");
    sizer->Add(addFrameButton);
    m_listCtrl = new FrameListCtrl(this, art.Get("close_frame"));
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

  void SetNumFrames(const index_t& numFrames){
    assert(numFrames.Get() != 0);
    m_listCtrl->SetNumFrames(numFrames);
    if ( numFrames.Get() == 1 ){
      m_listCtrl->Hide();
    }
    else{
      m_listCtrl->Show();
    }
  }
  FrameListCtrl* m_listCtrl;
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

void FrameControl::SetNumFrames( const index_t& numFrames ){
  m_impl->SetNumFrames(numFrames);
}
