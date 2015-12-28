// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#include "app/app-context.hh"
#include "commands/command-bunch.hh"
#include "commands/set-raster-selection-cmd.hh"
#include "geo/geo-func.hh"
#include "geo/rect.hh"
#include "geo/size.hh"
#include "objects/objraster.hh"
#include "tools/tool.hh"
#include "util/canvas.hh"
#include "util/clipboard.hh"
#include "util/command-util.hh"
#include "util/cut-and-paste.hh"
#include "util/image-props.hh"
#include "util/image-util.hh"
#include "util/image.hh"
#include "util/object-util.hh"
#include "util/paint.hh"
#include "util/setting-util.hh"

namespace faint{

static bool selection_to_clipboard(AppContext& app, const erase_copied& eraseCopied){
  Clipboard clipboard;
  if (!clipboard.Good()){
    return false;
  }
  Canvas& canvas = app.GetActiveCanvas();
  const Image& image = canvas.GetImage();
  ToolInterface& tool = canvas.GetTool();
  utf8_string text;
  bool copiedText = eraseCopied.Get() ? tool.CutText(text) : tool.CopyText(text);
  if (copiedText){
    clipboard.SetText(text);
    return true;
  }

  const RasterSelection& selection = canvas.GetRasterSelection();
  if (tool.GetLayerType() == Layer::RASTER && selection.Exists()){
    Paint bgSrc = app.GetToolSettings().Get(ts_Bg);
    Color bgCol = get_color_default(bgSrc, color_white());
    if (selection.Floating()){
      clipboard.SetBitmap(selection.GetBitmap(), strip_alpha(bgCol));
      if (eraseCopied.Get()){
        canvas.RunCommand(get_delete_raster_selection_command(image, bgSrc));
      }
    }
    else{
      // The non-floating selection can be dragged outside the image
      // with the right mouse, so clip it to the data
      IntRect rect(intersection(selection.GetRect(), image_rect(image)));
      if (empty(rect)){
        return false;
      }
      clipboard.SetBitmap(subbitmap(image, rect), strip_alpha(bgCol));
      if (eraseCopied.Get()){
        canvas.RunCommand(get_delete_rect_command(rect, bgSrc));
        canvas.Refresh();
      }
    }
    return true;
  }

  if (tool.GetLayerType() == Layer::OBJECT){
    const objects_t& objects = image.GetObjectSelection();
    if (objects.empty()){
      return false;
    }
    clipboard.SetObjects(objects);

    if (eraseCopied.Get()){
      canvas.RunCommand(get_delete_objects_command(objects, image, "Cut"));
      canvas.Refresh();
    }
    return true;
  }
  return false;
}

void cut_selection(AppContext& app){
  selection_to_clipboard(app, erase_copied(true));
}

void copy_selection(AppContext& app){
  selection_to_clipboard(app, erase_copied(false));
}

void paste_to_active_canvas(AppContext& app){
  Clipboard clipboard;
  if (!clipboard.Good()){
    return;
  }
  Canvas& canvas = app.GetActiveCanvas();
  ToolInterface& tool = canvas.GetTool();

  if (tool.AcceptsPastedText()){
    if (auto text = clipboard.GetText()){
      tool.Paste(text.Take());
    }
    else{
      // Stop further pasting when text paste failed. It would be
      // surprising if a bitmap was pasted while editing text.
      return;
    }
  }

  const bool rasterPaste = tool.GetLayerType() == Layer::RASTER;
  const bool objectPaste = !rasterPaste;
  if (objectPaste){
    if (auto maybeObjects = clipboard.GetObjects()){
      objects_t objects(maybeObjects.Take());

      // Find the correct offset to place the pasted objects
      // with relative positions intact and anchored at the view-start
      Point minObj = bounding_rect(objects).TopLeft();

      const Point viewStart = canvas.GetImageViewStart();
      offset_by(objects, viewStart - minObj);
      canvas.RunCommand(get_add_objects_command(objects, select_added(false), "Paste"));
      app.SelectTool(ToolId::OBJECT_SELECTION);
      canvas.SelectObjects(objects, deselect_old(true));
      return;
    }
  }

  if (auto maybeBmp = clipboard.GetBitmap()){
    if (rasterPaste){
      const RasterSelection& oldSelection(canvas.GetRasterSelection());
      commands_t commands;
      if (oldSelection.Floating()){
        commands.push_back(stamp_floating_selection_command(oldSelection));
      }
      Settings settings(app.GetToolSettings());
      commands.push_back(get_paste_raster_bitmap_command(maybeBmp.Take(),
          floored(canvas.GetImageViewStart()), oldSelection, settings));
      canvas.RunCommand(perhaps_bunch(CommandType::HYBRID, bunch_name(commands.back()->Name()), commands));
      app.SelectTool(ToolId::RECTANGLE_SELECTION);
    }
    else{
      Bitmap bitmap(std::move(maybeBmp.Take()));
      Settings s(default_raster_settings());
      s.Update(app.GetToolSettings());
      auto tri = tri_for_bmp(canvas.GetImageViewStart(), bitmap);
      Object* rasterObj = new ObjRaster(tri, bitmap, s);
      canvas.RunCommand(add_object_command(rasterObj, select_added(false),
        "Paste"));
      canvas.SelectObject(rasterObj, deselect_old(true));
      app.SelectTool(ToolId::OBJECT_SELECTION);
    }
    return;
  }

  // No suitable format on clipboard
  return;
}

void paste_to_new_image(AppContext& app){
  Clipboard clipboard;
  if (!clipboard.Good()){
    return;
  }

  if (auto bmp = clipboard.GetBitmap()){
    app.NewDocument(ImageProps(bmp.Take()));
    return;
  }

  if (auto maybeObjects = clipboard.GetObjects()){
    objects_t objects(maybeObjects.Take());
    auto objRect = bounding_rect(objects);
    offset_by(objects, -objRect.TopLeft());
    app.NewDocument(ImageProps(rounded(objRect.GetSize()), objects));
    return;
  }
}

}
