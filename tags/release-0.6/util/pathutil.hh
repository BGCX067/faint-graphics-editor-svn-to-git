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

#ifndef FAINT_HOMEDIR_HH
#define FAINT_HOMEDIR_HH
#include <string>

std::string get_home_dir();
std::string get_data_dir();
std::string get_palette_dir();
std::string get_help_dir();
std::string get_ini_file_path();
bool file_exists( const std::string& );
std::string get_full_path( const std::string& );
std::string join_path( const std::string&, const std::string& );
bool valid_save_filename( const std::string& );
#endif
