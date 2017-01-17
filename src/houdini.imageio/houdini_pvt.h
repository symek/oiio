/*
OpenImageIO and all code, documentation, and other materials contained
therein are:

Copyright 2010 Larry Gritz and the other authors and contributors.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of the software's owners nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(This is the Modified BSD License)
*/

#ifndef OPENIMAGEIO_HOUDINI_H
#define OPENIMAGEIO_HOUDINI_H

#include "IMG/IMG_File.h"

OIIO_PLUGIN_NAMESPACE_BEGIN

namespace houdini_pvt
{
 
// Houdini to OIIO type conversion  (prototyped after exrinput.cpp)
static TypeDesc
TypeDesc_from_HDKPixelType (IMG_DataType ptype)
{
    switch (ptype) {
    // case IMG_UCHAR   : return TypeDesc::UCHAR; break;
    case IMG_INT8    : return TypeDesc::INT8;  break;
    // case IMG_USHORT  : return TypeDesc::USHORT;break;
    case IMG_INT16   : return TypeDesc::INT16; break;
    // case IMG_UINT    : return TypeDesc::UINT;  break;
    case IMG_INT32   : return TypeDesc::INT32; break;
    // case IMG_FLOAT   : return TypeDesc::DOUBLE;break;
    case IMG_FLOAT32 : return TypeDesc::FLOAT; break;
    // case IMG_HALF    : return TypeDesc::HALF;  break;
    case IMG_FLOAT16 : return TypeDesc::HALF;  break;
    default: ASSERT_MSG (0, "Unknown IMG_DataType %d", int(ptype));
    }
}

}

OIIO_PLUGIN_NAMESPACE_END


#endif // OPENIMAGEIO_HOUDINI_H