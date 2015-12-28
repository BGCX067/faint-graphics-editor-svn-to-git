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

#ifndef FAINT_CANVASINTERFACE_HH
#define FAINT_CANVASINTERFACE_HH
#include "commands/command.hh"
#include "util/commonfwd.hh"
#include "util/idtypes.hh"
#include "util/path.hh"
#include "util/rasterselection.hh"
#include "util/statusinterface.hh"

class ToolInterface{
public:
  virtual bool Delete() = 0;
  virtual bool Deselect() = 0;
  virtual bool SelectAll() = 0;
  // Returns the layer type, taking the global layer type choice and
  // the tool in consideration.
  virtual Layer GetLayerType() const = 0;
};

class CanvasInterface {
public:
  virtual ~CanvasInterface(){}
  virtual void CenterView( const Point& ) = 0;
  virtual void ClearPointOverlay() = 0;
  virtual void ClearRasterSelectionChimera() = 0;
  virtual void CloseUndoBundle() = 0;
  virtual void ContextDelete() = 0;
  virtual void CopySelection() = 0;
  virtual void CutSelection() = 0;
  virtual void DeselectObject( Object* ) = 0;
  virtual void DeselectObjects() = 0;
  virtual void DeselectObjects(const objects_t&) = 0;
  virtual const faint::Bitmap& GetBitmap() = 0;
  virtual Optional<faint::FilePath> GetFilePath() const = 0;
  virtual const faint::Image& GetFrame(const index_t&) = 0;
  virtual const faint::Image& GetFrameById(const FrameId&) = 0;
  virtual index_t GetFrameIndex(const faint::Image&) const = 0;
  virtual Grid GetGrid() const = 0;
  virtual CanvasId GetId() const = 0;
  virtual const faint::Image& GetImage() = 0;
  virtual IntPoint GetMaxScrollPos() = 0;
  virtual index_t GetNumFrames() const = 0;
  virtual const objects_t& GetObjects() = 0;
  virtual const objects_t& GetObjectSelection() = 0;
  virtual CursorPositionInfo GetPosInfo( const IntPoint& ) = 0;
  virtual const RasterSelection& GetRasterSelection() = 0;
  virtual Point GetRelativeMousePos() = 0;
  virtual IntPoint GetScrollPos() = 0;
  virtual index_t GetSelectedFrame() const = 0;
  virtual IntSize GetSize() const = 0;
  virtual faint::Bitmap GetSubBitmap( const IntRect& ) const = 0;
  virtual ToolInterface& GetTool() = 0;
  virtual faint::coord GetZoom() const = 0;
  virtual ZoomLevel GetZoomLevel() const = 0;
  virtual bool Has( const FrameId& ) const = 0;
  virtual bool Has( const ObjectId& ) const = 0;
  virtual bool IsSelected( const Object*) const = 0;
  virtual void NextFrame() = 0;
  virtual void NotifySaved(const faint::FilePath&) = 0;
  virtual void OpenUndoBundle() = 0;
  virtual void Paste() = 0;
  virtual void PreviousFrame() = 0;
  virtual void Redo() = 0;
  virtual void Refresh() = 0;
  virtual void RunCommand( Command* ) = 0;
  virtual void RunCommand( Command*, const FrameId& ) = 0;
  virtual void RunDWIM() = 0;
  virtual void ScrollMaxDown() = 0;
  virtual void ScrollMaxLeft() = 0;
  virtual void ScrollMaxRight() = 0;
  virtual void ScrollMaxUp() = 0;
  virtual void ScrollPageDown() = 0;
  virtual void ScrollPageLeft() = 0;
  virtual void ScrollPageRight() = 0;
  virtual void ScrollPageUp() = 0;
  virtual void SelectFrame(const index_t&) = 0;
  virtual void SelectObject( Object*, const deselect_old& ) = 0;
  virtual void SelectObjects( const objects_t&, const deselect_old& ) = 0;
  virtual void SetChimera(RasterSelection*) = 0;
  virtual void SetGrid( const Grid& ) = 0;
  virtual void SetPointOverlay( const IntPoint& ) = 0;
  virtual void SetScrollPos( const IntPoint& ) = 0;
  virtual void SetZoom( const ZoomLevel& ) = 0;
  virtual void Undo() = 0;
  virtual void ZoomDefault() = 0;
  virtual void ZoomFit() = 0;
  virtual void ZoomIn() = 0;
  virtual void ZoomOut() = 0;
};

#endif
