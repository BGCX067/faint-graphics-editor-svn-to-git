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

#include "wx/wx.h"
#include "oneinstance.hh"
#include "getappcontext.hh"

bool FaintConnection::OnExec( const wxString&, const wxString& data ){
  wxString filename = data;
  GetAppContext().Load( filename, false );
  return true;
}

wxConnectionBase* FaintServer::OnAcceptConnection( const wxString& topic ){
  if ( topic.Lower() != "faint" ){
    return 0;
  }
  return new FaintConnection;
}

wxConnectionBase* FaintClient::OnMakeConnection(){
  return new FaintConnection;
}
