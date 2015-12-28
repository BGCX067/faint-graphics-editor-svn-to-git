#include "format_cur.hh"
#include "formats/msw_icon.hh"
#include "util/canvasinterface.hh"
#include "util/image.hh"
#include "util/imageutil.hh"

FormatCUR::FormatCUR()
  : Format(extension_t("cur"), label_t("Windows cursor (CUR)"), can_save(true), can_load(true))
{}

void FormatCUR::Load( const faint::FilePath& filename, ImageProps& imageProps ){
  faint::load_cursor( filename, imageProps );
}

SaveResult FormatCUR::Save( const faint::FilePath& filename, CanvasInterface& canvas ){
  std::vector<faint::Bitmap> bitmaps;
  std::vector<IntPoint> hotSpots;
  size_t numFrames(canvas.GetNumFrames().Get());
  for ( size_t i = 0; i != numFrames; i++ ){
    const faint::Image& frame(canvas.GetFrame(index_t(i)));
    bitmaps.push_back(faint::flatten(frame));
    hotSpots.push_back(frame.GetHotSpot());
  }
  return faint::save_cursor(filename, bitmaps, hotSpots);
}
