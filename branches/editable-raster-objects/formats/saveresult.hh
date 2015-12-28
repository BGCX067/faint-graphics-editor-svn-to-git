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
#ifndef FAINT_SAVERESULT
#define FAINT_SAVERESULT
#include <string>

// Fixme: must support utf-8 for filenames.
class SaveResult{
public:
  static SaveResult SaveSuccessful();
  static SaveResult SaveFailed( const std::string& error );
  bool Failed() const;
  bool Successful() const;
  std::string ErrorDescription() const;
private:
  SaveResult(bool, const std::string&);
  bool m_ok;
  std::string m_error;
};

#endif
