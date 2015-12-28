# Copyright 2012 Lukas Kemmer
#
# Licensed under the Apache License, Version 2.0 (the "License"); you
# may not use this file except in compliance with the License. You
# may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

base64_jpeg_prefix = "data:image/jpeg;base64,"
base64_png_prefix = "data:image/png;base64,"

def decode_base64( data ):
    import base64
    data = data.replace("\n", "")
    data = data.replace(" ", "")
    data = base64.b64decode(data)
    return data

def strip_MIME( data ):
    if data.startswith(base64_jpeg_prefix):
        return "base64_jpg", data[ len(base64_jpeg_prefix) : ]
    elif data.startswith(base64_png_prefix):
        return "base64_png", data[ len(base64_png_prefix) : ]
    else:
        return None, ""
