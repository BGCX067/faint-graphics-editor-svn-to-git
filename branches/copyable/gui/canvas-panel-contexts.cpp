// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#include "app/get-app-context.hh"
#include "commands/command.hh"
#include "geo/geo-func.hh"
#include "gui/canvas-panel.hh"
#include "gui/canvas-panel-contexts.hh"
#include "rendering/faint-dc.hh"
#include "rendering/paint-canvas.hh" // Fixme: for get_tool_layer
#include "util/image.hh"

namespace faint{

static bool aint_active_canvas(const CanvasPanel& canvas, AppContext& app){
  return app.GetActiveCanvas().GetId() != canvas.GetCanvasId();
}

class CommandContextImpl : public TargetableCommandContext{
public:
  CommandContextImpl(CanvasPanel& canvas, ImageList& images)
    : m_canvas(canvas),
      m_dc(nullptr),
      m_frame(nullptr),
      m_images(images)
  {}

  ~CommandContextImpl(){
    delete m_dc;
  }

  void Add(Object* obj, const select_added& select, const deselect_old& deselect) override {
    m_frame->Add(obj);
    if (select.Get()){
      m_canvas.SelectObject(obj, deselect);
    }
  }

  void Add(Object* obj, int z, const select_added& select, const deselect_old& deselect) override{
    m_frame->Add(obj, z);
    if (select.Get()){
      m_canvas.SelectObject(obj, deselect);
    }
  }

  void AddFrame(Image* image) override{
    m_images.AppendShared(image);
  }

  void AddFrame(Image* image, const index_t& index) override{
    m_images.InsertShared(image, index);
  }

  const Bitmap& GetBitmap() const override{
    if (const auto& bmp = m_frame->GetBg().Get<Bitmap>()){
      return bmp.Get();
    }
    return m_frame->CreateBackground();
  }

  FaintDC& GetDC() override{
    assert(m_frame != nullptr);
    if (m_dc == nullptr){
      m_dc = m_frame->GetBg().Visit(
        [](Bitmap& bmp){
          return new FaintDC(bmp);
        },
        [&](const ColorSpan&){
          return new FaintDC(m_frame->CreateBackground());
        });
    }

    // Fixme: Assertion won't work anymore
    // assert(m_dc->IsTargetting(m_frame->GetBitmap().Get()));
    return *m_dc;
  }

  Image& GetFrame() override{
    assert(m_frame != nullptr);
    return *m_frame;
  }

  Image& GetFrame(const index_t& index) override{
    return m_images.GetImage(index);
  }

  IntSize GetImageSize() const override{
    return m_frame->GetSize();
  }

  const objects_t& GetObjects() override{
    return m_frame->GetObjects();
  }

  int GetObjectZ(const Object* obj) override{
    return m_frame->GetObjectZ(obj);
  }

  RasterSelection& GetRasterSelection() override{
    return m_frame->GetRasterSelection();
  }

  Bitmap& GetRawBitmap() override{
    return m_frame->GetBg().Visit(
      [](Bitmap& bmp) -> Bitmap&{
        return bmp;
      },
      [&](const ColorSpan&) -> Bitmap&{
        return m_frame->CreateBackground();
      });
  }

  bool HasObjects() const override{
    return m_frame->GetNumObjects() != 0;
  }

  void MoveRasterSelection(const IntPoint& topLeft) override{
    RasterSelection& selection(m_canvas.GetImageSelection());
    assert(selection.Exists());
    selection.Move(topLeft);
  }

  void OffsetRasterSelectionOrigin(const IntPoint& delta) override{
    RasterSelection& selection(m_frame->GetRasterSelection());
    if (selection.Exists()){
      selection.OffsetOrigin(delta);
    }
    Grid g = m_canvas.GetGrid();
    g.SetAnchor(g.Anchor() + floated(delta));
    m_canvas.SetGrid(g);
  }

  void Remove(Object* obj) override{
    m_canvas.DeselectObject(obj);
    m_frame->Remove(obj);
  }

  void RemoveFrame(const index_t& index) override{
    m_images.Remove(index);
  }

  void RemoveFrame(Image* image) override{
    m_images.Remove(image);
  }

  void ReorderFrame(const new_index_t& newIndex, const old_index_t& oldIndex) override{
    m_images.Reorder(newIndex, oldIndex);
  }

  void SetBitmap(const Bitmap& bmp) override{
    Reset();
    m_frame->SetBitmap(bmp);
  }

  void SetBitmap(Bitmap&& bmp) override{
    Reset();
    m_frame->SetBitmap(std::move(bmp));
  }

  void SetObjectZ(Object* obj, int z) override{
    m_frame->SetObjectZ(obj, z);
  }

  void SetRasterSelection(const SelectionState& state) override{
    m_frame->GetRasterSelection().SetState(state);
  }

  void SetRasterSelectionOptions(const SelectionOptions& options) override{
    m_frame->GetRasterSelection().SetOptions(options);
  }

  void RevertFrame() override{
    Reset();
    m_frame->Revert();
  }
private:
  void SetFrame(Image* frame) override{
    Reset();
    m_frame = frame;
  }

  // Non virtual
  void Reset(){
    delete m_dc;
    m_dc = nullptr;
  }
  CanvasPanel& m_canvas;
  FaintDC* m_dc;
  Image* m_frame;
  ImageList& m_images;
};

class CanvasToolInterface : public ToolInterface{
public:
  CanvasToolInterface(CanvasPanel& canvas, AppContext& app)
    : m_canvas(canvas),
      m_appContext(app)
  {}

  bool AcceptsPastedText() const override{
    return m_canvas.m_contexts.GetTool().AcceptsPastedText();
  }

  bool CopyText(utf8_string& text) override{
    return m_canvas.m_contexts.GetTool().CopyText(text, erase_copied(false));
  }

  bool CutText(utf8_string& text) override{
    if (m_canvas.m_contexts.GetTool().CopyText(text, erase_copied(true))){
      m_canvas.RefreshToolRect();
      return true;
    }
    return false;
  }

  bool Delete() override{
    if (aint_active_canvas(m_canvas, m_appContext)){
      return false;
    }
    Tool& tool = m_canvas.m_contexts.GetTool();
    if (!tool.SupportsSelection()){
      return false;
    }
    m_canvas.HandleToolResult(tool.Delete());
    return true;
  }

  bool Deselect() override{
    if (aint_active_canvas(m_canvas, m_appContext)){
      return false;
    }
    Tool& tool = m_canvas.m_contexts.GetTool();
    if (!tool.SupportsSelection()){
      return false;
    }
    m_canvas.HandleToolResult(tool.Deselect());
    return true;
  }

  void Paste(const utf8_string& text) override{
    m_canvas.m_contexts.GetTool().Paste(text);
    m_canvas.RefreshToolRect();
  }

  bool SelectAll() override{
    if (aint_active_canvas(m_canvas, m_appContext)){
      return false;
    }
    Tool& tool = m_canvas.m_contexts.GetTool();
    if (!tool.SupportsSelection()){
      return false;
    }
    return m_canvas.HandleToolResult(tool.SelectAll());
  }

  bool SupportsSelection() const override{
    return m_canvas.m_contexts.GetTool().SupportsSelection();
  }

  Layer GetLayerType() const override{
    ToolId tool = m_canvas.m_contexts.tool.GetToolId();
    Layer layer = m_appContext.GetLayerType();
    return get_tool_layer(tool, layer);
  }
private:
  CanvasPanel& m_canvas;
  AppContext& m_appContext;
};

class CanvasContext : public Canvas {
public:
  CanvasContext(CanvasPanel& canvas, AppContext& app, const ImageList& images)
    : m_canvas(canvas),
      m_images(images),
      m_toolInterface(canvas, app)
  {}

  void CenterView(const Point& pt) override{
    m_canvas.CenterViewImage(pt);
  }

  void ClearPointOverlay() override{
    m_canvas.ClearPointOverlay();
  }

  void CloseUndoBundle() override{
    m_canvas.CloseUndoBundle();
  }

  void DeselectObject(Object* obj) override{
    m_canvas.DeselectObject(obj);
  }

  void DeselectObjects() override{
    m_canvas.DeselectObjects();
  };

  void DeselectObjects(const objects_t& objs) override{
    m_canvas.DeselectObjects(objs);
  }

  const Either<Bitmap, ColorSpan>& GetBackground() const override{
    return m_images.Active().GetBg();
  }

  const Optional<Bitmap>& GetBitmap() const override{
    return m_images.Active().GetBg().Get<Bitmap>();
  }

  Optional<FilePath> GetFilePath() const override{
    return m_canvas.GetFilePath();
  }

  const Image& GetFrame(const index_t& index) override{
    return m_canvas.GetImageList().GetImage(index);
  }

  const Image& GetFrameById(const FrameId& id) override{
    return m_canvas.GetImageList().GetImageById(id);
  }

  index_t GetFrameIndex(const Image& image) const override{
    return m_canvas.GetImageList().GetIndex(image);
  }

  Grid GetGrid() const override{
    return m_canvas.GetGrid();
  }

  CanvasId GetId() const override{
    return m_canvas.GetCanvasId();
  }

  const Image& GetImage() const override{
    return m_images.Active();
  }

  Point GetImageViewStart() const override{
    return m_canvas.GetImageViewStart();
  }

  IntPoint GetMaxScrollPos() override{
    return m_canvas.GetMaxUsefulScroll();
  }

  index_t GetNumFrames() const override{
    return m_canvas.GetImageList().GetNumImages();
  }

  const objects_t& GetObjects() override{
    return m_canvas.GetObjects();
  }

  PosInfo GetPosInfo(const IntPoint& pos) override{
    return m_canvas.ExternalHitTest(pos);
  }

  const RasterSelection& GetRasterSelection() const override{
    return m_images.Active().GetRasterSelection();
  }

  Point GetRelativeMousePos() override{
    return m_canvas.GetRelativeMousePos();
  }

  IntPoint GetScrollPos() override{
    return m_canvas.GetFaintScrollPos();
  }

  index_t GetSelectedFrame() const override{
    return m_canvas.GetSelectedFrame();
  }

  const objects_t& GetObjectSelection() const override{
    return m_canvas.GetObjectSelection();
  }

  IntSize GetSize() const override{
    return m_images.Active().GetSize();
    // return m_canvas.GetImage().GetSize();
  }

  ToolInterface& GetTool() override{
    return m_toolInterface;
  }

  const ToolInterface& GetTool() const override{
    return m_toolInterface;
  }

  coord GetZoom() const override{
    return m_canvas.GetZoom();
  }

  ZoomLevel GetZoomLevel() const override{
    return m_canvas.GetZoomLevel();
  }

  bool Has(const ObjectId& id) const override{
    return m_canvas.Has(id);
  }

  bool Has(const FrameId& id) const override{
    return m_canvas.GetImageList().Has(id);
  }

  void NextFrame() override{
    m_canvas.NextFrame();
  }

  void NotifySaved(const FilePath& filePath) override{
    m_canvas.NotifySaved(filePath);
  }

  void OpenUndoBundle() override{
    m_canvas.OpenUndoBundle();
  }

  void PreviousFrame() override{
    m_canvas.PreviousFrame();
  }

  void Redo() override{
    m_canvas.Redo();
  }

  void Refresh() override{
    m_canvas.Refresh();
  }

  void RunCommand(Command* cmd) override{
    if (cmd != nullptr){
      // Preempt so that for example(!) a move-object tool is
      // preempted before the moved objects are removed.  This is a
      // bit harsh, it should be possible to trigger commands without
      // disabling tools, but this requires more advanced handling,
      // e.g. checking if the tool is disturbed by the effects of the
      // command.
      m_canvas.Preempt(PreemptOption::ALLOW_COMMAND);
      m_canvas.RunCommand(cmd);
    }
  }

  void RunCommand(Command* cmd, const FrameId& id) override{
    if (cmd == nullptr){
      return;
    }
    // Preempt so that for example(!) a move-object tool is
    // preempted before the moved objects are removed.  This is a
    // bit harsh, it should be possible to trigger commands without
    // disabling tools, but this requires more advanced handling,
    // e.g. checking if the tool is disturbed by the effects of the
    // command.
    m_canvas.Preempt(PreemptOption::ALLOW_COMMAND);
    m_canvas.RunCommand(cmd, id);
  }

  void RunDWIM() override{
    m_canvas.RunDWIM();
  }

  void ScrollMaxDown() override{
    m_canvas.ScrollMaxDown();
  }

  void ScrollMaxLeft() override{
    m_canvas.ScrollMaxLeft();
  }

  void ScrollMaxRight() override{
    m_canvas.ScrollMaxRight();
  }

  void ScrollMaxUp() override{
    m_canvas.ScrollMaxUp();
  }

  void ScrollPageDown() override{
    m_canvas.ScrollPageDown();
  }

  void ScrollPageLeft() override{
    m_canvas.ScrollPageLeft();
  }

  void ScrollPageRight() override{
    m_canvas.ScrollPageRight();
  }

  void ScrollPageUp() override{
    m_canvas.ScrollPageUp();
  }

  void SelectFrame(const index_t& index) override{
    m_canvas.SelectFrame(index);
  }

  void SelectObject(Object* obj, const deselect_old& deselectOld) override{
    m_canvas.SelectObject(obj, deselectOld);
  }

  void SelectObjects(const objects_t& objs, const deselect_old& deselectOld) override{
    m_canvas.SelectObjects(objs, deselectOld);
  }

  void SetGrid(const Grid& g) override{
    m_canvas.SetGrid(g);
  }

  void SetMirage(const std::weak_ptr<Copyable<Bitmap>>& bmp) override{
    m_canvas.SetMirage(bmp);
  }

  void SetMirage(const std::weak_ptr<RasterSelection>& selection) override{
    m_canvas.SetMirage(selection);
  }

  void SetPointOverlay(const IntPoint& p) override{
    m_canvas.SetPointOverlay(p);
  }

  void SetScrollPos(const IntPoint& p) override{
    m_canvas.SetFaintScrollPos(p);
  }

  void SetZoom(const ZoomLevel& zoom) override{
    m_canvas.SetZoomLevel(zoom);
  }

  void Undo() override{
    m_canvas.Undo();
  }

  void ZoomDefault() override{
    m_canvas.ChangeZoom(ZoomLevel::DEFAULT);
  }

  void ZoomFit() override{
    m_canvas.ZoomFit();
  }

  void ZoomIn() override{
    m_canvas.ChangeZoom(ZoomLevel::NEXT);
  }

  void ZoomOut() override{
    m_canvas.ChangeZoom(ZoomLevel::PREVIOUS);
  }
private:
  CanvasPanel& m_canvas;
  const ImageList& m_images;
  CanvasToolInterface m_toolInterface;
};

TargetableCommandContext* create_command_context(CanvasPanel& canvas, ImageList& images){
  return new CommandContextImpl(canvas, images);
}

Canvas* create_canvas_context(CanvasPanel& panel, AppContext& app, const ImageList& images){
  return new CanvasContext(panel, app, images);
}

}
