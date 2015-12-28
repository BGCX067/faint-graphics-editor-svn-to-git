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
#include "util/rasterselection.hh"
#include "util/statusinterface.hh"
#include "util/unique.hh"

enum ApplyTarget{
  APPLY_OBJECT_SELECTION,
  APPLY_RASTER_SELECTION,
  APPLY_IMAGE
};

struct CursorPositionInfo; // Fixme: use class

class CanvasInterface {
public:
  virtual void BundleUndo( size_t count, const std::string& name ) = 0;
  virtual void CenterView( const Point& ) = 0;
  virtual void ClearDirty() = 0;
  virtual void ClearPointOverlay() = 0;
  virtual void ContextCrop() = 0;
  virtual void ContextDelete() = 0;
  virtual void ContextDeselect() = 0;
  virtual void ContextFlatten() = 0;
  virtual void ContextFlipHorizontal() = 0;
  virtual void ContextFlipVertical() = 0;
  virtual void ContextRotate90CW() = 0;
  virtual void ContextSelectAll() = 0;
  virtual void DeselectObject( Object* ) = 0;
  virtual void DeselectObjects() = 0;
  virtual void DeselectObjects(const objects_t&) = 0;
  virtual const faint::Bitmap& GetBitmap() = 0;
  virtual std::string GetFilename() const = 0;
  virtual const faint::Image& GetFrame(size_t) = 0;
  virtual int GetFrameDelay( size_t ) const = 0;
  virtual const faint::ImageList& GetFrames() const = 0;
  virtual Grid GetGrid() const = 0;
  virtual CanvasId GetId() const = 0;
  virtual const faint::Image& GetImage() = 0;
  virtual size_t GetNumFrames() const = 0;
  virtual const objects_t& GetObjects() = 0;
  virtual const objects_t& GetObjectSelection() = 0;
  virtual CursorPositionInfo GetPosInfo( const IntPoint& ) = 0;
  virtual const RasterSelection& GetRasterSelection() = 0;
  virtual Point GetRelativeMousePos() = 0;
  virtual size_t GetSelectedFrame() const = 0;
  virtual IntSize GetSize() const = 0;
  virtual faint::Bitmap GetSubBitmap( const IntRect& ) const = 0;
  virtual faint::coord GetZoom() const = 0;
  virtual ZoomLevel GetZoomLevel() const = 0;
  virtual bool Has( const ObjectId& ) const = 0;
  virtual bool IsSelected( const Object*) const = 0;
  virtual void NextFrame() = 0;
  virtual void PreviousFrame() = 0;
  virtual void Redo() = 0;
  virtual void Refresh() = 0;
  virtual void ClearRasterSelectionChimera() = 0;
  virtual void RunCommand( Command* ) = 0;
  virtual void RunDWIM() = 0;
  virtual void ScrollMaxDown() = 0;
  virtual void ScrollMaxLeft() = 0;
  virtual void ScrollMaxRight() = 0;
  virtual void ScrollMaxUp() = 0;
  virtual void ScrollPageDown() = 0;
  virtual void ScrollPageLeft() = 0;
  virtual void ScrollPageRight() = 0;
  virtual void ScrollPageUp() = 0;
  virtual void SelectFrame(size_t) = 0;
  virtual void SelectObject( Object*, const deselect_old& ) = 0;
  virtual void SelectObjects( const objects_t&, const deselect_old& ) = 0;
  virtual void SetChimera(RasterSelection*) = 0;
  virtual void SetFilename( const std::string& ) = 0;
  virtual void SetGrid( const Grid& ) = 0;
  virtual void SetPointOverlay( const IntPoint& ) = 0;
  virtual void SetRasterSelection( const IntRect& ) = 0;
  virtual void SetZoom( const ZoomLevel& ) = 0;
  virtual void Undo() = 0;
  virtual void ZoomDefault() = 0;
  virtual void ZoomFit() = 0;
  virtual void ZoomIn() = 0;
  virtual void ZoomOut() = 0;
};

#endif
