// -*- coding: us-ascii-unix -*-
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

#ifndef FAINT_FORMAT_HH
#define FAINT_FORMAT_HH
#include <string>
#include "formats/save-result.hh"
#include "util/common-fwd.hh"
#include "util/distinct.hh"
#include "util/file-path.hh"

namespace faint{
class Format;
typedef Distinct<utf8_string, Format, 0> label_t;
typedef Distinct<bool, Format, 0> can_save;
typedef Distinct<bool, Format, 1> can_load;

class Format{
  // Base class for file formats
public:
  Format(const FileExtension&,
    const label_t&, const can_save&, const can_load&);
  Format(const std::vector<FileExtension>&,
    const label_t&, const can_save&, const can_load&);
  virtual ~Format();
  bool CanSave() const;
  bool CanLoad() const;
  const FileExtension& GetDefaultExtension() const;
  const std::vector<FileExtension>& GetExtensions() const;
  const utf8_string& GetLabel() const;
  bool Match(const FileExtension&) const;
  virtual void Load(const FilePath&, ImageProps&) = 0;
  virtual SaveResult Save(const FilePath&, Canvas&) = 0;
private:
  bool m_canLoad;
  bool m_canSave;
  std::vector<FileExtension> m_extensions;
  utf8_string m_label;
};

}

#endif
