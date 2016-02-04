/******************************************************************************
* This file is part of a tool for producing 3D content in the PRC format.
* Copyright (c) 2013-2015, Bradley J Chambers, brad.chambers@gmail.com
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <hpdf.h>

#include <prc/oPRCFile.hpp>

#include <pdal/pdal_export.hpp>
#include <pdal/Writer.hpp>

namespace
{
class BOX3D;
}

namespace pdal
{

enum OUTPUT_FORMAT
{
    OUTPUT_FORMAT_PDF,
    OUTPUT_FORMAT_PRC
};

enum COLOR_SCHEME
{
    COLOR_SCHEME_SOLID,
    COLOR_SCHEME_ORANGES,
    COLOR_SCHEME_BLUE_GREEN
};

enum CONTRAST_STRETCH
{
    CONTRAST_STRETCH_LINEAR,
    CONTRAST_STRETCH_SQRT
};

class PDAL_DLL PrcWriter : public Writer
{
public:
    PrcWriter();

    static void * create();
    static int32_t destroy(void *);
    std::string getName() const;

    Options getDefaultOptions();

private:
    virtual void initialize();
    virtual void processOptions(const Options& options);
    virtual void ready(PointTableRef table);
    virtual void write(const PointViewPtr view);
    virtual void done(PointTableRef table);

    std::unique_ptr<oPRCFile> m_prcFile;
    std::string m_prcFilename;
    std::string m_pdfFilename;
    BOX3D m_bounds;

    int m_outputFormat;
    int m_colorScheme;
    int m_contrastStretch;

    HPDF_REAL m_fov;
    HPDF_REAL m_coox;
    HPDF_REAL m_cooy;
    HPDF_REAL m_cooz;
    HPDF_REAL m_c2cx;
    HPDF_REAL m_c2cy;
    HPDF_REAL m_c2cz;
    HPDF_REAL m_roo;
    HPDF_REAL m_roll;

    PrcWriter& operator=(const PrcWriter&); // not implemented
    PrcWriter(const PrcWriter&); // not implemented

};

} // namespace pdal
