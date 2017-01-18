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
    virtual bool open (const std::string &name, ImageSpec &newspec, const ImageSpec &config);
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
    bool copy_metadata(ImageSpec &spec);
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
    int m_components;
    bool m_keep_unassociated_alpha;
    bool m_deep;




    std::vector<PartInfo> m_parts;        /// < Image parts
    std::string    m_filename;           /// < Stash the filename
    IMG_File      *m_hdk_scan_file;      /// < Open image handle
    IMG_File      *m_hdk_tile_file;      /// < Tiled interface version
    // IMG_FileParms *m_hdk_parms;
    // IMG_FileParms *m_hdk_parms_tile;
    IMG_Stat      *m_hdk_stat;
    IMG_Format    *m_hdk_format;
    std::unique_ptr<char[]> tilebuf;
    std::shared_ptr<IMG_FileParms> m_hdk_parms_tile;
    std::shared_ptr<IMG_FileParms> m_hdk_parms;
     
    /// Reset everything to initial state
     void init (void) {
        // m_scanline_size = 0;
        m_hdk_scan_file = NULL;
        m_hdk_tile_file = NULL;
        m_filename.clear ();
        m_keep_unassociated_alpha=false;
        tilebuf.reset();
    } 


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



bool HoudiniInput::open(const std::string &name, ImageSpec &newspec, 
                        const ImageSpec &config) 
{
    if (config.get_int_attribute("oiio:UnassociatedAlpha", 0) == 1)
        m_keep_unassociated_alpha = true; // not sure...

    return open(name, newspec);
}

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

    m_hdk_parms.reset(new IMG_FileParms());
    m_hdk_parms->flipImageVertical(); // Houdini starts with lower/left, OIIO upper/left
    m_hdk_scan_file  = IMG_File::open(m_filename.c_str(), m_hdk_parms.get());

    if (!m_hdk_scan_file) {
        error ("Could not open file \"%s\"", name.c_str());
        return false;
    }

    m_hdk_format = m_hdk_scan_file->getFormat();
   
    // We keep separate copy of IMG_File for tile based access and probably
    // for deeps (unless we separete whole *.cpp for this.)
    if (std::string("RAT").compare(m_hdk_format->getFormatName()) == 0) {
        m_hdk_parms_tile.reset(new IMG_FileParms());
        m_hdk_parms_tile->flipImageVertical();
        m_hdk_parms_tile->useTileInterface();
        m_hdk_tile_file = IMG_File::open(m_filename.c_str(),  m_hdk_parms_tile.get());
        if (!m_hdk_tile_file)
            return false;
        if(m_hdk_tile_file->getImageType() == IMG_TYPE_DEEP_PIXEL) {
            m_deep = true;
        }
    }
       
    m_spec       = ImageSpec();
    m_hdk_stat   = &(m_hdk_scan_file->getStat());
    m_nsubimages = m_hdk_stat->getNumPlanes();

    // OIIO sub-images will be used for every extra channel,
    // since RAT/PIC hold channels in seprate rasters varying components, depth, ordering.
    m_parts.resize(m_nsubimages);

    PartInfo &info(m_parts[0]);
    info.nmiplevels = 0; // FIXME: Something fishy here. 

    // return true;
    return seek_subimage(0, 0, newspec);
}


bool HoudiniInput::copy_metadata(ImageSpec &spec) 
{
    std::string attr("houdini:");
    for (int i = 0; i < m_hdk_scan_file->getNumOptions(); i++) {
        spec.attribute(attr + m_hdk_scan_file->getOptionName(i), \
            m_hdk_scan_file->getOptionValue(i));
    }

    UT_SharedPtr<UT_Options> opt;
    if (opt = m_hdk_scan_file->imageTextureOptions()) {
        UT_Options::iterator it;
        for (it=opt->begin(); it != opt->end(); it.advance()) {
            const std::string name(it.name());
            const UT_OptionType type = opt->getOptionType(name);

            if (type == UT_OPTION_MATRIX4) {
                const UT_Matrix4D mat = opt->getOptionM4(name);
                spec.attribute(attr + name, TypeDesc::TypeMatrix44, mat.data());
            }
            if (type == UT_OPTION_MATRIX3) {
                const UT_Matrix3D mat = opt->getOptionM3(name);
                spec.attribute(attr + name, TypeDesc::TypeMatrix33, mat.data());
            }
            else if (type == UT_OPTION_STRING) {
                std::string str;
                opt->getOptionS(name, str);
                spec.attribute(attr + name, str);
            }
            else if (type == UT_OPTION_FPREAL) {
                const double d = opt->getOptionF(name);
                spec.attribute(attr + name, TypeDesc::DOUBLE, static_cast<const void*>(&d));   
            }
             else if (type == UT_OPTION_INT) {
                const int i = opt->getOptionI(name);
                spec.attribute(attr + name, i);
            }
            else if (type == UT_OPTION_VECTOR2) {
                const UT_Vector2D d = opt->getOptionV2(name);
                TypeDesc v2(TypeDesc::DOUBLE, TypeDesc::VEC2);
                spec.attribute(attr + name, v2, static_cast<const void*>(&d));  
            }
            else if (type == UT_OPTION_VECTOR3) {
                const UT_Vector3D d = opt->getOptionV3(name);
                TypeDesc v3(TypeDesc::DOUBLE, TypeDesc::VEC3);
                spec.attribute(attr + name, v3, static_cast<const void*>(&d));
            }
        }
    }

    return true;
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

    // if (m_hdk_tile_file){
    //     spec.tile_width  = 32;
    //     spec.tile_height = 32;
    //     spec.tile_depth  = 1;
    // }

    spec.deep = (m_hdk_scan_file->getImageType() == IMG_TYPE_DEEP_PIXEL);

    //copy metadata
    // NOTE: I currently don't provide metadata mapping because I need to figure out
    // OIIO convention. 
    copy_metadata(spec);
    
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
    delete m_hdk_scan_file;
    if (m_hdk_tile_file)
        delete m_hdk_tile_file; 
    init(); 
    return true;
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


bool HoudiniInput::read_native_scanline (int y, int z, void *data)
{
    assert(m_subimage < m_hdk_stat->getNumPlanes());
    const IMG_Plane *plane  = m_hdk_stat->getPlane(m_subimage); 
    return m_hdk_scan_file->readIntoBuffer(y, data, plane);
}

// bool HoudiniInput::read_native_tile (int x, int y, int z, void *data)
// {
//     assert(m_subimage < m_hdk_stat->getNumPlanes()); 
//     const IMG_Plane *plane  = m_hdk_tile_file->getStat().getPlane(m_subimage);
//     UT_InclusiveRect rect(x, y, x+m_spec.tile_width, y+m_spec.tile_height);
//     m_hdk_parms_tile->useTileInterface();
//     tilebuf.reset(new char [m_spec.tile_bytes(false)]);
//     data = &tilebuf[0];
//     return m_hdk_tile_file->readTile(rect, data, plane);
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