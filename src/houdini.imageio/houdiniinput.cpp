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

#include <cstdio>
#include <cstdlib>
#include <cmath>


#include "OpenImageIO/dassert.h"
#include "OpenImageIO/typedesc.h"
#include "OpenImageIO/imageio.h"
#include "OpenImageIO/filesystem.h"
#include "OpenImageIO/fmath.h"


#include "houdini_pvt.h"


OIIO_PLUGIN_NAMESPACE_BEGIN

using namespace houdini_pvt;

class HoudiniInput : public ImageInput 
{
public:
    HoudiniInput () { init(); }
    virtual ~HoudiniInput () { close(); }
    virtual const char * format_name (void) const { return "Houdini (pic/rat)"; }
    virtual bool valid_file (const std::string &filename) const;
    virtual bool open (const std::string &name, ImageSpec &newspec);
    virtual bool close ();
    // virtual int current_subimage (void) const { return m_subimage; }
    // virtual int current_miplevel (void) const { return 0;/*FIXME: tmp*/ }
    virtual bool seek_subimage (int subimage, int miplevel, ImageSpec &newspec);
    virtual bool read_native_scanline (int y, int z, void *data);
    // virtual bool read_native_scanlines (int ybegin, int yend, int z, void *data);
    // virtual bool read_native_scanlines (int ybegin, int yend, int z,
    //                                     int firstchan, int nchans, void *data);
    // virtual bool read_native_tile (int x, int y, int z, void *data);
    // virtual bool read_native_tiles (int xbegin, int xend, int ybegin, int yend,
    //                                 int zbegin, int zend, void *data);
    // virtual bool read_native_tiles (int xbegin, int xend, int ybegin, int yend,
    //                                 int zbegin, int zend,
    //                                 int firstchan, int nchans, void *data);
private:
    struct PartInfo {
        bool initialized;
        ImageSpec spec;
        int topwidth;                     ///< Width of top mip level
        int topheight;                    ///< Height of top mip level
        int levelmode;                    ///< The level mode
        int roundingmode;                 ///< Rounding mode
        bool cubeface;                    ///< It's a cubeface environment map
        int nmiplevels;                   ///< How many MIP levels are there?
        // Imath::Box2i top_datawindow;
        // Imath::Box2i top_displaywindow;
        std::vector<TypeDesc> pixeltype; ///< Imf pixel type for each chan
        std::vector<int> chanbytes;       ///< Size (in bytes) of each channel
        PartInfo () : initialized(false) { }
        ~PartInfo () { }
        // void parse_header (const Imf::Header *header);
        // void query_channels (const Imf::Header *header);
    };

        // char *m_input_multipart;   ///< Multipart input
        // char *m_scanline_input_part;
        // char *m_tiled_input_part;
        // char *m_deep_scanline_input_part;
        // char *m_deep_tiled_input_part;

    // Imf::InputFile *m_input_scanline;     ///< Input for scanline files
    // Imf::TiledInputFile *m_input_tiled;   ///< Input for tiled files
    int m_subimage;                       ///< What subimage are we looking at?
    int m_nsubimages;                     ///< How many subimages are there?
    int m_miplevel;                       ///< What MIP level are we looking at?



    std::vector<PartInfo> m_parts;        ///< Image parts

    std::string    m_filename;           ///< Stash the filename
    IMG_File      *m_hdk_file;              ///< Open image handle
    IMG_FileParms *m_hdk_parms;
    IMG_Stat      *m_hdk_stat;
    IMG_Format    *m_hdk_format;
    // std::string      m_file; 

    // int m_subimage;
    // int m_miplevel;
    int m_components;
    /// Reset everything to initial state
     void init (void) {
        // m_scanline_size = 0;
        m_hdk_file = NULL;
        m_filename.clear ();
    } //m_file = NULL;


};

// symbols required for OpenImageIO plugin
OIIO_PLUGIN_EXPORTS_BEGIN
    OIIO_EXPORT int houdini_imageio_version = OIIO_PLUGIN_VERSION;
    OIIO_EXPORT ImageInput *houdini_input_imageio_create() {
        return new HoudiniInput;
    }
    
    OIIO_EXPORT const char *houdini_input_extensions[] = {
        "rat", "pic", NULL
    };

OIIO_PLUGIN_EXPORTS_END




bool HoudiniInput::open(const std::string &name, ImageSpec &newspec) 
{
     // Quick check to reject non-exr files
    if (! Filesystem::is_regular (name)) {
        error ("Could not open file \"%s\"", name.c_str());
        return false;
    }

    // 
    m_subimage = -1;
    m_miplevel = -1;
    m_filename = name;

    m_hdk_file = IMG_File::open(m_filename.c_str());// TODO: allow read-time parms: m_parms, m_format
    if (!m_hdk_file)
        return false;

    m_spec       = ImageSpec();
    m_hdk_stat   = &(m_hdk_file->getStat());
    m_nsubimages = m_hdk_stat->getNumPlanes();

    // OIIO sub-images will be used for every extra channel,
    // since RAT/PIC hold channels in seprate rasters varying components, depth, ordering.
    m_parts.resize(m_nsubimages);

    PartInfo &info(m_parts[0]);
    info.nmiplevels = 0; // FIXME: Something fishy here. 

    // return true;
    return seek_subimage(0, 0, newspec);
}


 bool HoudiniInput::seek_subimage (int subimage, int miplevel, ImageSpec &newspec)
{

    PartInfo &info(m_parts[subimage]);

    if (subimage < 0 || subimage >= m_nsubimages)   // out of range
        return false;

    if (subimage == m_subimage && miplevel == m_miplevel) {  // no change
        newspec = m_spec;
        return true;
    }

    if (miplevel < 0 || miplevel > info.nmiplevels)   // out of range
        return false;
  
    if (miplevel == 0 && info.initialized /*&& info.levelmode == Imf::ONE_LEVEL*/) { // TODO: implement levelnodes etc
        newspec = m_spec;
        return true;
    }

    //
    const IMG_Plane *plane   = m_hdk_stat->getPlane(subimage);
    const TypeDesc datatype = TypeDesc_from_HDKPixelType(plane->getDataType());
    const int components    =  plane->getComponentCount();
    const int pixelsize     =  plane->getPixelSize();

    ImageSpec spec = ImageSpec(m_hdk_stat->getXres(), m_hdk_stat->getYres(), components, datatype);
    spec.attribute ("XResolution", m_hdk_stat->getXres());
    spec.attribute ("YResolution", m_hdk_stat->getYres());
    spec.attribute ("ResolutionUnit", "m");

    for(uint c=0; c<components; ++c)
    {
        std::string channel_name(plane->getName());
        if (components > 1) //getComponentName() crashes for single channle planes
            spec.channelnames.push_back(channel_name + "." + plane->getComponentName(c));
        else
            spec.channelnames.push_back(channel_name);

        info.pixeltype.push_back(datatype);  // same for all channels in HDK (but varying for planes).
        info.chanbytes.push_back(pixelsize); // as above.
    }

    info.spec = spec;
    info.initialized = true;
    info.topwidth = m_hdk_stat->getXres();
    info.topheight = m_hdk_stat->getYres();
    info.nmiplevels = 0;
    
    newspec = spec;
    m_spec  = spec;
    m_subimage = subimage;
    m_miplevel = miplevel;
    return true;

}


bool HoudiniInput::close() 
{ 
    delete m_hdk_file; 
    init(); 
    return true;
}




bool HoudiniInput::read_native_scanline (int y, int z, void *data)
{
    assert(m_subimage < m_hdk_stat->getNumPlanes());
    const IMG_Plane *plane  = m_hdk_stat->getPlane(m_subimage); 
    return m_hdk_file->readIntoBuffer(y, data, plane);
}


bool HoudiniInput::valid_file (const std::string &filename) const
{

    FILE *fd = Filesystem::fopen (filename, "rb");
    if (!fd)
        return false;

    fclose(fd);

    IMG_File *tmp = IMG_File::open(filename.c_str()); // TODO: do magic with magic number?

    if (!tmp)
    {
        error ("\"%s\" is not a file, Houdini can open.", filename.c_str());
        return false;
    }

    delete tmp;
    return true;
}

// bool HoudiniInput::seek_subimage (int subimage, int miplevel, ImageSpec &newspec)
// {
//     if (subimage < 0 || subimage >= m_nsubimages)   // out of range
//         return false;

//     if (subimage == m_subimage && miplevel == m_miplevel) {  // no change
//         newspec = m_spec;
//         return true;
//     }

//     PartInfo &part (m_parts[subimage]);

//     if (! part.initialized) {
//         const Imf::Header *header = NULL;
// #ifdef USE_OPENEXR_VERSION2
//         if (m_input_multipart)
//             header = &(m_input_multipart->header(subimage));
// #else
//         if (m_input_tiled)
//             header = &(m_input_tiled->header());
//         if (m_input_scanline)
//             header = &(m_input_scanline->header());
// #endif
//         part.parse_header (header);
//         part.initialized = true;
//     }

// #ifdef USE_OPENEXR_VERSION2
//     if (subimage != m_subimage) {
//         delete m_scanline_input_part;  m_scanline_input_part = NULL;
//         delete m_tiled_input_part;  m_tiled_input_part = NULL;
//         delete m_deep_scanline_input_part;  m_deep_scanline_input_part = NULL;
//         delete m_deep_tiled_input_part;  m_deep_tiled_input_part = NULL;
//         try {
//             if (part.spec.deep) {
//                 if (part.spec.tile_width)
//                     m_deep_tiled_input_part = new Imf::DeepTiledInputPart (*m_input_multipart, subimage);
//                 else
//                     m_deep_scanline_input_part = new Imf::DeepScanLineInputPart (*m_input_multipart, subimage);
//             } else {
//                 if (part.spec.tile_width)
//                     m_tiled_input_part = new Imf::TiledInputPart (*m_input_multipart, subimage);
//                 else
//                     m_scanline_input_part = new Imf::InputPart (*m_input_multipart, subimage);
//             }
//         } catch (const std::exception &e) {
//             error ("OpenEXR exception: %s", e.what());
//             m_scanline_input_part = NULL;
//             m_tiled_input_part = NULL;
//             m_deep_scanline_input_part = NULL;
//             m_deep_tiled_input_part = NULL;
//             return false;
//         } catch (...) {   // catch-all for edge cases or compiler bugs
//             error ("OpenEXR exception: unknown");
//             m_scanline_input_part = NULL;
//             m_tiled_input_part = NULL;
//             m_deep_scanline_input_part = NULL;
//             m_deep_tiled_input_part = NULL;
//             return false;
//         }
//     }
// #endif

//     m_subimage = subimage;

//     if (miplevel < 0 || miplevel >= part.nmiplevels)   // out of range
//         return false;

//     m_miplevel = miplevel;
//     m_spec = part.spec;

//     if (miplevel == 0 && part.levelmode == Imf::ONE_LEVEL) {
//         newspec = m_spec;
//         return true;
//     }

//     // Compute the resolution of the requested mip level.
//     int w = part.topwidth, h = part.topheight;
//     if (part.levelmode == Imf::MIPMAP_LEVELS) {
//         while (miplevel--) {
//             if (part.roundingmode == Imf::ROUND_DOWN) {
//                 w = w / 2;
//                 h = h / 2;
//             } else {
//                 w = (w + 1) / 2;
//                 h = (h + 1) / 2;
//             }
//             w = std::max (1, w);
//             h = std::max (1, h);
//         }
//     } else if (part.levelmode == Imf::RIPMAP_LEVELS) {
//         // FIXME
//     } else {
//         ASSERT_MSG (0, "Unknown levelmode %d", int(part.levelmode));
//     }

//     m_spec.width = w;
//     m_spec.height = h;
//     // N.B. OpenEXR doesn't support data and display windows per MIPmap
//     // level.  So always take from the top level.
//     Imath::Box2i datawindow = part.top_datawindow;
//     Imath::Box2i displaywindow = part.top_displaywindow;
//     m_spec.x = datawindow.min.x;
//     m_spec.y = datawindow.min.y;
//     if (m_miplevel == 0) {
//         m_spec.full_x = displaywindow.min.x;
//         m_spec.full_y = displaywindow.min.y;
//         m_spec.full_width = displaywindow.max.x - displaywindow.min.x + 1;
//         m_spec.full_height = displaywindow.max.y - displaywindow.min.y + 1;
//     } else {
//         m_spec.full_x = m_spec.x;
//         m_spec.full_y = m_spec.y;
//         m_spec.full_width = m_spec.width;
//         m_spec.full_height = m_spec.height;
//     }
//     if (part.cubeface) {
//         m_spec.full_width = w;
//         m_spec.full_height = w;
//     }
//     newspec = m_spec;

//     return true;
// }

//      m_spec = ImageSpec(); // Clear everything with default constructor
    
//     try {
//         m_input_stream = new OpenEXRInputStream (name.c_str());
//     } catch (const std::exception &e) {
//         m_input_stream = NULL;
//         error ("OpenEXR exception: %s", e.what());
//         return false;
//     } catch (...) {   // catch-all for edge cases or compiler bugs
//         m_input_stream = NULL;
//         error ("OpenEXR exception: unknown");
//         return false;
//     }

// #ifdef USE_OPENEXR_VERSION2
//     try {
//         m_input_multipart = new Imf::MultiPartInputFile (*m_input_stream);
//     } catch (const std::exception &e) {
//         delete m_input_stream;
//         m_input_stream = NULL;
//         error ("OpenEXR exception: %s", e.what());
//         return false;
//     } catch (...) {   // catch-all for edge cases or compiler bugs
//         m_input_stream = NULL;
//         error ("OpenEXR exception: unknown");
//         return false;
//     }

//     m_nsubimages = m_input_multipart->parts();

// #else
//     try {
//         if (tiled) {
//             m_input_tiled = new Imf::TiledInputFile (*m_input_stream);
//         } else {
//             m_input_scanline = new Imf::InputFile (*m_input_stream);
//         }
//     } catch (const std::exception &e) {
//         delete m_input_stream;
//         m_input_stream = NULL;
//         error ("OpenEXR exception: %s", e.what());
//         return false;
//     } catch (...) {   // catch-all for edge cases or compiler bugs
//         m_input_stream = NULL;
//         error ("OpenEXR exception: unknown");
//         return false;
//     }

//     if (! m_input_scanline && ! m_input_tiled) {
//         error ("Unknown error opening EXR file");
//         return false;
//     }

//     m_nsubimages = 1;  // OpenEXR 1.x did not have multipart
// #endif

//     m_parts.resize (m_nsubimages);
//     m_subimage = -1;
//     m_miplevel = -1;
//     bool ok = seek_subimage (0, 0, newspec);
//     if (! ok)
//         close ();
//     return ok;
// }







// bool HoudiniInput::seek_subimage (int subimage, int miplevel, ImageSpec &newspec)
// {
//   return true;
// }


// bool HoudiniInput::read_native_scanlines (int ybegin, int yend, int z, void *data) 
// {
//   return true;
// }

// bool HoudiniInput::read_native_scanlines (int ybegin, int yend, int z,
//                                         int firstchan, int nchans, void *data)
// {
//   return true;
// }
// bool HoudiniInput::read_native_tile (int x, int y, int z, void *data)
// {
//   return true;
// }
// bool HoudiniInput::read_native_tiles (int xbegin, int xend, int ybegin, int yend,
//                                     int zbegin, int zend, void *data)
// {
//   return true;
// }
// bool HoudiniInput::read_native_tiles (int xbegin, int xend, int ybegin, int yend,
//                                     int zbegin, int zend,
//                                     int firstchan, int nchans, void *data)
// {
//   return true;
// }



OIIO_PLUGIN_NAMESPACE_END