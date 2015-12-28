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
#include "wx/ipc.h"
#include "wx/snglinst.h"
#include <sstream>
#include "oneinstance.hh"
#include "getappcontext.hh"

const std::string g_defaultFaintPort("3793"); // Fixme: Consider 49152+ default
const wxString g_faintTopicFiles("faint-files");
const wxString g_faintTopicRaise("faint-raise");
const wxString& g_filePrefix = "file:";
const wxString& g_messageComplete = "done";

bool is_file_message( const wxString& message ){
  if ( message.size() < g_filePrefix.size() ){
    return false;
  }
  return message.substr(0, g_filePrefix.size() ) == g_filePrefix;
}

bool is_completion_message( const wxString& message ){
  return message == g_messageComplete;
}

std::string get_file_name( const wxString& message ){
  assert(is_file_message(message));
  return std::string(message.substr(g_filePrefix.size()));
}

class FaintConnection : public wxConnection {
public:
  bool OnExec( const wxString& /*topic*/, const wxString& message ){
    if ( is_file_message(message) ){
      m_files.push_back( get_file_name(message) );
    }
    else if ( is_completion_message(message) ){
      // QueueLoadFiles returns immediately, this avoids the sending
      // application from timing out, and showing a wxWidgets error
      // message.  (The sender doesn't care if the files load or not -
      // the other Faint instance should deal with the error
      // reporting)
      GetAppContext().QueueLoad(m_files);
      m_files.clear();
    }
    return true;
  }
private:
  std::vector<std::string> m_files;
};

class FaintClient : public wxClient {
public:
  wxConnectionBase* OnMakeConnection(){
    return new FaintConnection;
  }
};

class FaintServer : public wxServer {
public:
  wxConnectionBase* OnAcceptConnection( const wxString& topic ){
    if ( topic == g_faintTopicFiles){
      return new FaintConnection;
    }
    else if ( topic == g_faintTopicRaise ){
      GetAppContext().RaiseFrame();
      return new FaintConnection;
    }
    else {
      return 0;
    }
  }
};

void show_start_server_error(const wxString& port){
    std::stringstream ss;
    ss << "Failed to create an IPC-service" << std::endl << std::endl << "Using a single instance of Faint, and passing file names to the running instance will not work." << std::endl << "Port: " << port;
    wxLogError(ss.str().c_str());
}

wxServer* start_faint_server(const std::string& port){
  FaintServer* server = new FaintServer;
  bool ok = server->Create(port);
  if ( !ok ){
    delete server;
    show_start_server_error(port);
    return 0;
  }
  return server;
}

bool send_files_to_server( const std::vector<std::string>& files, const std::string& port ){
  FaintClient* client = new FaintClient();
  wxString hostname = "localhost";

  wxConnectionBase* connection = client->MakeConnection( hostname, port, g_faintTopicFiles);
  if ( connection == 0 ){
    // Failed connecting to the old instance
    delete client;
    return false;
  }

  // Pass all the files to the old instance
  for ( size_t i = 0; i!= files.size(); i++ ){
    connection->Execute( g_filePrefix + files[i] );
  }
  connection->Execute( g_messageComplete );
  connection->Disconnect();
  delete client;
  delete connection;
  return true;
}

bool raise_existing_window(const std::string& port){
  FaintClient* client = new FaintClient();
  wxString hostname = "localhost";
  wxConnectionBase* connection = 0;

  int attempt = 0;
  while ( attempt < 3 && connection == 0 ){
    connection = client->MakeConnection( hostname, port, g_faintTopicRaise);
    attempt++;
    if ( connection == 0 ){
      wxThread::Sleep( 500 );
    }
  }

  if ( connection == 0 ){
    // Connecting to the old instance failed.
    delete client;
    return false;
  }

  connection->Disconnect();
  delete client;
  delete connection;
  return true;
}


FaintInstance::~FaintInstance(){
}

typedef Unique<bool, FaintInstance, 10> allow_start;
class FaintInstanceImpl : public FaintInstance{
public:
  FaintInstanceImpl( wxServer* server, wxSingleInstanceChecker* singleInst, const allow_start& allowStart)
    : m_allowStart(allowStart.Get()),
      m_server(server),
      m_singleInstance(singleInst)
  {  }

  ~FaintInstanceImpl(){
    delete m_server;
    delete m_singleInstance;
    m_server = 0;
    m_singleInstance = 0;
  }

  bool AllowStart() const{
    return m_allowStart;
  }
private:
  bool m_allowStart;
  wxServer* m_server;
  wxSingleInstanceChecker* m_singleInstance;
};

wxSingleInstanceChecker* get_instance_checker(){
  const wxString name = wxString::Format("Faint-%s", wxGetUserId().c_str());
  return new wxSingleInstanceChecker(name);
}

FaintInstance* create_faint_instance(const std::vector<std::string>& cmdLineFiles, const allow_server& allowServer, const force_start& forceStart, const std::string& port){
  // Create the first instance, if possible
  wxSingleInstanceChecker* singleInst = get_instance_checker();
  if ( !singleInst->IsAnotherRunning() ){
    wxServer* server = 0;
    if ( allowServer.Get() ){
      server = start_faint_server(port);
    }
    else{
      delete singleInst;
      singleInst = 0;
    }
    return new FaintInstanceImpl( server, singleInst, allow_start(true) );
  }

  if ( forceStart.Get() ){
    return new FaintInstanceImpl(0, 0, allow_start(true));
  }

  if ( cmdLineFiles.empty() ){
    // Faint is running since previously, so just raise the window.
    if ( raise_existing_window(port) ){
      return new FaintInstanceImpl(0, 0, allow_start(false));
    }
    // Failed raising old Faint - something is amiss, allow a new
    // instance.
    return new FaintInstanceImpl(0, 0, allow_start(true));
  }

  // Faint is running since previously and should be passed the
  // filenames from this instance.
  if ( send_files_to_server(cmdLineFiles, port) ){
    // The files were sent to the running instance. Prevent
    // this instance from starting.
    return new FaintInstanceImpl(0,0, allow_start(false));
  }
  else {
    // Failed sending files to old instance - something is amiss,
    // allow a new instance.
    return new FaintInstanceImpl(0,0, allow_start(true));
  }
  assert(false); // Unreachable
  return 0;
}
