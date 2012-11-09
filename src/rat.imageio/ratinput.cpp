/*
  Copyright 2009 Larry Gritz and the other authors and contributors.
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

// #include <cstdio>
// #include <cstdlib>
// #include <cmath>


// #include "dassert.h"
// #include "typedesc.h"
#include "imageio.h"
// #include "thread.h"
// #include "strutil.h"
// #include "fmath.h"

#include "IMG/IMG_File.h"


OIIO_PLUGIN_NAMESPACE_BEGIN

class RatInput : public ImageInput 
{
public:
    RatInput () { init(); }
    virtual ~RatInput () { close(); }
    virtual const char * format_name (void) const { return "rat"; }
    virtual bool open (const std::string &name, ImageSpec &newspec);
    virtual bool close ();
    virtual int current_subimage (void) const { return m_subimage; }
    virtual int current_miplevel (void) const { return m_miplevel; }
    virtual bool seek_subimage (int subimage, int miplevel, ImageSpec &newspec);
    virtual bool read_native_scanline (int y, int z, void *data);
    virtual bool read_native_scanlines (int ybegin, int yend, int z, void *data);
    virtual bool read_native_scanlines (int ybegin, int yend, int z,
                                        int firstchan, int nchans, void *data);
    virtual bool read_native_tile (int x, int y, int z, void *data);
    virtual bool read_native_tiles (int xbegin, int xend, int ybegin, int yend,
                                    int zbegin, int zend, void *data);
    virtual bool read_native_tiles (int xbegin, int xend, int ybegin, int yend,
                                    int zbegin, int zend,
                                    int firstchan, int nchans, void *data);
private:
    std::string m_filename;           ///< Stash the filename
    IMG_File *m_file;                 ///< Open image handle
    /// Reset everything to initial state
    void init () { m_file = NULL; }

};



// Obligatory material to make this a recognizeable imageio plugin:
OIIO_PLUGIN_EXPORTS_BEGIN
OIIO_EXPORT ImageInput *rat_input_imageio_create () { return new RatInput; }
OIIO_EXPORT int rat_imageio_version = OIIO_PLUGIN_VERSION;
OIIO_EXPORT const char * rat_input_extensions[] = {"rat", NULL};
OIIO_PLUGIN_EXPORTS_END
OIIO_PLUGIN_NAMESPACE_END