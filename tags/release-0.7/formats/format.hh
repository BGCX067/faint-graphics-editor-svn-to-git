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
#include "formats/saveresult.hh"
#include "util/commonfwd.hh"
#include "util/unique.hh"

class Format;
typedef Unique<std::string, Format, 0> extension_t;
typedef Unique<std::string, Format, 1> label_t;
typedef Unique<bool, Format, 0> can_save;
typedef Unique<bool, Format, 1> can_load;

class Format{
  // Base class for file formats
public:
  Format( const extension_t&, const label_t& label, const can_save&, const can_load& );
  Format( const std::vector<extension_t>&, const label_t& label, const can_save&, const can_load& );
  virtual ~Format();
  bool CanSave() const;
  bool CanLoad() const;
  const std::string& GetDefaultExtension() const;
  const std::string& GetLabel() const;
  bool Match( const extension_t& ) const;
  virtual void Load( const std::string& filename, ImageProps& ) = 0;
  virtual SaveResult Save( const std::string& filename, CanvasInterface& ) = 0;
private:
  bool m_canLoad;
  bool m_canSave;
  std::vector<std::string> m_extensions;
  std::string m_label;
};

#endif
