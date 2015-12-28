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
#include "formats/format_gif.hh"
#include "formats/format_wx.hh"
#include "util/canvasinterface.hh"
#include "util/convertwx.hh"
#include "util/formatting.hh"
#include "util/image.hh"
#include "util/imageutil.hh"
#include "wx/anidecod.h" // wxImageArray
#include "wx/quantize.h"
#include "wx/filename.h"
#include "wx/wfstream.h"
#include "wx/wx.h"

size_t find_mismatch_index( const std::vector<IntSize>& sizes ){
  assert(!sizes.empty());
  size_t i = 1;
  for ( ; i != sizes.size(); i++ ){
    if ( sizes[i] != sizes[0] ){
      break;
    }
  }
  return i;
}

bool uniform_size( const std::vector<IntSize>& sizes ){
  return find_mismatch_index(sizes) == sizes.size();
}

SaveResult fail_size_mismatch( const std::vector<IntSize>& sizes ){
  size_t index = find_mismatch_index(sizes);
  assert(index != sizes.size());
  std::stringstream ss;
  ss << "This image can not be saved as a gif." << std::endl << std::endl <<
    "It contains frames of different sizes." << std::endl <<
    "Frame 1: " << str(sizes[0]) << std::endl <<
    "Frame " << index + 1 << ": " << str(sizes[index]);
  return SaveResult::SaveFailed(ss.str());
}

std::vector<IntSize> get_frame_sizes(CanvasInterface& canvas){
  std::vector<IntSize> sizes;
  size_t numFrames = canvas.GetNumFrames().Get();
  for ( size_t i = 0; i != numFrames; i++ ){
    sizes.push_back(canvas.GetFrame(index_t(i)).GetSize());
  }
  return sizes;
}

FormatGIF::FormatGIF()
  : Format( extension_t("gif"), label_t("Graphics Interchange Format (GIF)"), can_save(true), can_load(true))
{}

void FormatGIF::Load( const faint::FilePath& filePath, ImageProps& imageProps ){
  faint::load_file_wx(filePath, wxBITMAP_TYPE_GIF, imageProps);
}

SaveResult FormatGIF::Save( const faint::FilePath& filePath, CanvasInterface& canvas){
  std::vector<IntSize> sizes = get_frame_sizes(canvas);
  if ( !uniform_size(sizes) ){
    return fail_size_mismatch(sizes);
  }

  wxImageArray images;
  size_t numFrames(canvas.GetNumFrames().Get());
  for ( size_t i = 0; i != numFrames; i++ ){
    faint::Bitmap bmp(faint::flatten(canvas.GetFrame(index_t(i))));
    wxImage img(to_wx_image(bmp));
    wxQuantize::Quantize(img, img);
    images.Add( new wxImage(img) ); // AFAIK, wxImageArray uses delete
  }

  wxGIFHandler handler;
  wxFileOutputStream out(filePath.ToWx().GetLongPath()); // Fixme: Check result?

  // Fixme: SaveAnimation only supports a single delay
  int delay_ms = canvas.GetFrame(index_t(0)).GetDelay().Get();
  bool ok = handler.SaveAnimation(images, &out, false, delay_ms);
  if ( ok ){
    return SaveResult::SaveSuccessful();
  }
  return SaveResult::SaveFailed("");
}
