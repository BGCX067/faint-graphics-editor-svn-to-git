#ifndef FAINT_FORMAT_CUR_HH
#define FAINT_FORMAT_CUR_HH
#include "formats/format.hh"

class FormatCUR : public Format {
public:
  FormatCUR();
  void Load( const faint::FilePath&, ImageProps& ) override;
  SaveResult Save( const faint::FilePath&, CanvasInterface& ) override;
private:
};

#endif
