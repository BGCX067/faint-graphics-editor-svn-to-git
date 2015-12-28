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

#ifndef FAINT_ONEINSTANCE_HH
#define FAINT_ONEINSTANCE_HH
#include "util/unique.hh"
#include "util/commonfwd.hh"

class FaintInstance{
public:
  virtual ~FaintInstance();
  virtual bool AllowStart() const = 0;
};

typedef Unique<bool, FaintInstance, 0> allow_server;
typedef Unique<bool, FaintInstance, 1> force_start;
FaintInstance* create_faint_instance(const faint::FileList& cmdLineFiles,
  const allow_server&,
  const force_start&,
  const std::string& port);

extern const std::string g_defaultFaintPort;

#endif
