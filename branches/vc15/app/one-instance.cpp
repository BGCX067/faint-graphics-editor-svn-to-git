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

#include <sstream>
#include "wx/filename.h"
#include "wx/ipc.h"
#include "wx/snglinst.h"
#include "app/get-app-context.hh"
#include "app/one-instance.hh"
#include "util-wx/convert-wx.hh"
#include "util-wx/file-path.hh"

namespace faint{

const wxString g_faintTopicFiles("faint-files");
const wxString g_faintTopicRaise("faint-raise");
const wxString& g_filePrefix = "file:";
const wxString& g_messageComplete = "done";

static bool is_file_message(const wxString& message){
  if (message.size() < g_filePrefix.size()){
    return false;
  }
  return message.substr(0, g_filePrefix.size()) == g_filePrefix;
}

static bool is_completion_message(const wxString& message){
  return message == g_messageComplete;
}

static wxFileName extract_file_path(const wxString& message){
  assert(is_file_message(message));
  wxString path(message.substr(g_filePrefix.size()));
  return wxFileName(path);
}

class FaintConnection : public wxConnection {
public:
  bool OnExec(const wxString& /*topic*/, const wxString& message) override{
    if (is_file_message(message)){
      // A file message from a different instance
      wxFileName pathWx(extract_file_path(message));
      if (pathWx.IsAbsolute() && !pathWx.IsDir()){
        m_files.push_back(FilePath::FromAbsoluteWx(pathWx));
      }
      else{
        // Do nothing on bad file name
      }
    }
    else if (is_completion_message(message)){
      // QueueLoadFiles returns immediately, this avoids the sending
      // application from timing out, and showing a wxWidgets error
      // message.  (The sender doesn't care if the files load or not -
      // the other Faint instance should deal with the error
      // reporting)
      get_app_context().QueueLoad(m_files);
      m_files.clear();
    }
    return true;
  }
private:
  FileList m_files;
};

class FaintClient : public wxClient {
public:
  wxConnectionBase* OnMakeConnection() override{
    return new FaintConnection;
  }
};

class FaintServer : public wxServer {
public:
  wxConnectionBase* OnAcceptConnection(const wxString& topic) override{
    if (topic == g_faintTopicFiles){
      return new FaintConnection;
    }
    else if (topic == g_faintTopicRaise){
      get_app_context().RaiseWindow();
      return new FaintConnection;
    }
    else {
      return nullptr;
    }
  }
};

static void show_start_server_error(const wxString& port){
    std::stringstream ss;
    ss << "Failed to create an IPC-service" << std::endl << std::endl << "Using a single instance of Faint, and passing file names to the running instance will not work." << std::endl << "Port: " << port;
    wxLogError(ss.str().c_str());
}

static wxServer* start_faint_server(const std::string& port){
  FaintServer* server = new FaintServer;
  bool ok = server->Create(port);
  if (!ok){
    delete server;
    show_start_server_error(port);
    return nullptr;
  }
  return server;
}

static bool send_paths_to_server(const FileList& paths, const std::string& port){
  FaintClient* client = new FaintClient();
  wxString hostname = "localhost";

  wxConnectionBase* connection = client->MakeConnection(hostname, port, g_faintTopicFiles);
  if (connection == nullptr){
    // Failed connecting to the old instance
    delete client;
    return false;
  }

  // Pass all the files to the old instance
  for (const FilePath& filePath : paths){
    connection->Execute(g_filePrefix + to_wx(filePath.Str()));
  }
  connection->Execute(g_messageComplete);
  connection->Disconnect();
  delete client;
  delete connection;
  return true;
}

static bool raise_existing_window(const std::string& port){
  FaintClient* client = new FaintClient();
  wxString hostname = "localhost";
  wxConnectionBase* connection = nullptr;

  int attempt = 0;
  while (attempt < 3 && connection == nullptr){
    connection = client->MakeConnection(hostname, port, g_faintTopicRaise);
    attempt++;
    if (connection == nullptr){
      wxThread::Sleep(500);
    }
  }

  if (connection == nullptr){
    // Connecting to the old instance failed.
    delete client;
    return false;
  }

  connection->Disconnect();
  delete client;
  delete connection;
  return true;
}

using allow_start = Distinct<bool, FaintInstance, 10>;

class FaintInstanceImpl : public FaintInstance{
public:
  FaintInstanceImpl(wxServer* server, wxSingleInstanceChecker* singleInst, const allow_start& allowStart)
    : m_allowStart(allowStart.Get()),
      m_server(server),
      m_singleInstance(singleInst)
  {}

  ~FaintInstanceImpl(){
    delete m_server;
    delete m_singleInstance;
    m_server = nullptr;
    m_singleInstance = nullptr;
  }

  bool AllowStart() const override{
    return m_allowStart;
  }
private:
  bool m_allowStart;
  wxServer* m_server;
  wxSingleInstanceChecker* m_singleInstance;
};

static wxSingleInstanceChecker* get_instance_checker(){
  const wxString name = wxString::Format("Faint-%s", wxGetUserId().c_str());
  return new wxSingleInstanceChecker(name);
}

std::unique_ptr<FaintInstance> create_faint_instance(const FileList& cmdLineFiles,
  const allow_server& allowServer,
  const force_start& forceStart,
  const std::string& port)
{
  // Create the first instance, if possible
  wxSingleInstanceChecker* singleInst = get_instance_checker();
  if (!singleInst->IsAnotherRunning()){
    wxServer* server = nullptr;
    if (allowServer.Get()){
      server = start_faint_server(port);
    }
    else{
      delete singleInst;
      singleInst = nullptr;
    }
    return std::make_unique<FaintInstanceImpl>(server,
      singleInst,
      allow_start(true));
  }

  if (forceStart.Get()){
    return std::make_unique<FaintInstanceImpl>(nullptr,
      nullptr,
      allow_start(true));
  }

  if (cmdLineFiles.empty()){
    // Faint is running since previously, so just raise the window.
    if (raise_existing_window(port)){
      return std::make_unique<FaintInstanceImpl>(nullptr,
        nullptr,
        allow_start(false));
    }
    // Failed raising old Faint - something is amiss, allow a new
    // instance.
    return std::make_unique<FaintInstanceImpl>(nullptr,
      nullptr,
      allow_start(true));
  }

  // Faint is running since previously and should be passed the
  // filenames from this instance.
  if (send_paths_to_server(cmdLineFiles, port)){
    // The files were sent to the running instance. Prevent
    // this instance from starting.
    return std::make_unique<FaintInstanceImpl>(nullptr,
      nullptr,
      allow_start(false));
  }

  // Failed sending paths to old instance - something is amiss,
  // allow a new instance.
  return std::make_unique<FaintInstanceImpl>(nullptr,
    nullptr,
    allow_start(true));
}

} // namespace
