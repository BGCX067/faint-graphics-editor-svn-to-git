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
#include "wx/ipc.h"

class FaintConnection : public wxConnection {
public:
  bool OnExec( const wxString& topic, const wxString& data );  
};


class FaintServer : public wxServer {
public:
  wxConnectionBase* OnAcceptConnection( const wxString& topic );
};


class FaintClient : public wxClient {
public:
  wxConnectionBase* OnMakeConnection();
};


#endif
