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
#include "commands/frame-cmd.hh"
#include "gui/cursors.hh"
#include "gui/framectrl.hh"
#include "util/artcontainer.hh"
#include "util/convertwx.hh"
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
      dropPost(0),
      sourceFrame(0)
  {}

  FrameDragInfo(wxPoint in_dragStart, const index_t& in_sourceFrame)
    : active(false),
      copy(false),
      dragStart(in_dragStart),
      dropPost(0),
      sourceFrame(in_sourceFrame)
  {}

  index_t DropFrame() const{
    if ( copy ){
      // When copying, the index for the new frame is the same as the
      // index of the post.
      return dropPost;
    }

    // When a frame is moved, a drop on either post surrounding the
    // source frame maps to the original frame index. Therefore, when
    // dragging rightwards, the post-count must be lowered by one to
    // get the resulting frame index.
    return index_t( dropPost - (sourceFrame < dropPost ? 1 : 0 ) );
  }

  bool WouldChange() const{
    return active && ( copy || DropFrame() != sourceFrame );
  }

  bool active;
  bool copy;
  wxPoint dragStart;
  index_t dropPost;
  index_t sourceFrame;
};

FrameDragInfo then_reset( FrameDragInfo& info ){
  // Resets the drag info by default construction, returns the state
  // as a copy.
  FrameDragInfo oldState;
  std::swap( oldState, info );
  return oldState;
}

static void run_frame_drag_command( CanvasInterface& canvas, const FrameDragInfo& dragInfo ){
  index_t srcIndex(dragInfo.sourceFrame);
  index_t dstIndex(dragInfo.DropFrame());
  if ( dragInfo.copy ){
    canvas.RunCommand(add_frame_command(canvas.GetFrame(srcIndex), dstIndex));
  }
  else {
    canvas.RunCommand(reorder_frame_command(New(dstIndex), Old(srcIndex)));
  }
  canvas.SelectFrame(dstIndex);
}

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
    const int iconSpacing = 5;
    const int iconWidth = m_frameBoxSize.GetWidth();
    const int iconHeight = m_frameBoxSize.GetHeight();
    const int panelHeight = 40;
    m_bitmap = wxBitmap(wxSize(( iconWidth + 2 * iconSpacing ) * m_numFrames.Get(), panelHeight));
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
      wxPoint pos( i * (iconWidth + iconSpacing) + iconSpacing, 0 );
      dc.DrawRectangle( pos, m_frameBoxSize );
      if ( i == selected ){
        dc.DrawBitmap( m_closeFrameBitmap, pos + wxPoint(iconWidth - m_closeFrameBitmap.GetWidth() - 1, 1),
          true ); // Use mask
      }
    }
    if ( m_dragInfo.active ){
      wxPen dragPen(wxColour(0,0,0), 3, wxSOLID);
      dragPen.SetCap(wxCAP_BUTT);
      dc.SetPen(dragPen);
      int dropPost = m_dragInfo.dropPost.Get();
      dc.DrawLine( dropPost * (iconWidth + iconSpacing) + iconSpacing - 3, 0,
        dropPost * (iconWidth + iconSpacing) + iconSpacing - 3, iconHeight );
    }
  }

  void SetNumFrames( const index_t& numFrames ){
    SetInitialSize(wxSize(40 * numFrames.Get() + 10,40));
    m_numFrames = numFrames;
    Refresh();
  }

  void OnCaptureLost( wxMouseCaptureLostEvent& ){
    // Discard any drag and refresh this control to get rid of drag
    // overlay graphics
    if ( then_reset(m_dragInfo).active ){
      Refresh();
    }
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
    }

    FrameDragInfo dragResult( then_reset(m_dragInfo) );
    if ( dragResult.WouldChange() ){
      run_frame_drag_command( GetAppContext().GetActiveCanvas(), dragResult );
      // No refresh is required, this control will be updated due to
      // the frame reordering)
    }
    else{
      // Refresh to clear drop the overlay
      Refresh();
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
        if ( ctrlHeld ){
          if ( !m_dragInfo.copy ){
            m_dragInfo.copy = true;
            SetCursor(to_wx_cursor(Cursor::DRAG_COPY_FRAME));
          }
        }
        else if ( m_dragInfo.copy ){
          SetCursor(to_wx_cursor(Cursor::DRAG_FRAME));
        }

        index_t frame = GetFrameIndex( event.GetPosition(), GetAppContext().GetActiveCanvas(), true );
        if ( m_dragInfo.dropPost != frame ){
          m_dragInfo.dropPost = frame;
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
EVT_MOUSE_CAPTURE_LOST(FrameListCtrl::OnCaptureLost)
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
    addFrameButton->SetBitmap(art.Get(Icon::ADD_FRAME));
    addFrameButton->SetToolTip("Add Frame");
    sizer->Add(addFrameButton);
    m_listCtrl = new FrameListCtrl(this, art.Get(Icon::CLOSE_FRAME));
    sizer->Add(m_listCtrl);
    SetSizer(sizer);
    Layout();

    GetEventHandler()->Connect(wxID_ANY,
      wxEVT_COMMAND_BUTTON_CLICKED,
      wxCommandEventHandler(FrameControlImpl::OnButton),
      nullptr,
      this);
  }

  void OnButton(wxCommandEvent&){
    CanvasInterface& canvas(GetAppContext().GetActiveCanvas());
    canvas.RunCommand(add_frame_command(canvas.GetSize()));
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
  m_impl = nullptr; // Deletion is handled by wxWidgets
}

wxWindow* FrameControl::AsWindow(){
  return m_impl;
}

void FrameControl::SetNumFrames( const index_t& numFrames ){
  m_impl->SetNumFrames(numFrames);
}
