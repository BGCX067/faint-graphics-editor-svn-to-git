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

#ifndef FAINT_TABCONTROL_HH
#define FAINT_TABCONTROL_HH
#include "canvasinterface.hh"

class TabControlImpl;

wxString GetFilename( const wxString& path );

class CanvasScroller;
class TabControl {
public:
  TabControl( wxWindow* parent );
  CanvasScroller* NewDocument( ImageProps&, bool changePage=true );
  CanvasScroller* NewDocument( const CanvasInfo&, bool changePage=true ); // Fixme: Remove
  CanvasScroller* NewDocument( const CanvasInfo&, const std::vector<Object*>&, bool changePage=true ); // Fixme: Remove
  CanvasScroller* NewDocument( wxString title, const faint::Bitmap&, bool changePage=false); // Fixme: Remove
  CanvasScroller* GetActiveCanvas();
  void Close( size_t page );
  CanvasScroller* GetCanvas( size_t page );
  size_t GetCanvasCount();
  void SelectNext();
  void SelectPrevious();
  void CloseActive();
  bool UnsavedDocuments() const;
  wxWindow* Window();
  void RefreshActiveTabName();
  void RefreshTabName(const CanvasScroller* const );
  void ShowWindowMenu();
  void ShowTabs(bool);
private:
  TabControlImpl* pImpl;
};

#endif
