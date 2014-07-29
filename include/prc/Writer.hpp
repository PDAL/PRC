/******************************************************************************
* This file is part of a tool for producing 3D content in the PRC format.
* Copyright (c) 2013, Bradley J Chambers, brad.chambers@gmail.com
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

#ifndef INCLUDED_DRIVERS_PRC_WRITER_HPP
#define INCLUDED_DRIVERS_PRC_WRITER_HPP

#include <string>

#include <boost/cstdint.hpp>

#include <hpdf.h>

#include <prc/oPRCFile.hpp>

#include <pdal/pdal_internal.hpp>
#include <pdal/Bounds.hpp>
#include <pdal/Writer.hpp>
#include <pdal/FileUtils.hpp>
#include <pdal/StageFactory.hpp>


namespace pdal
{
namespace drivers
{
namespace prc
{


class prc_driver_error : public pdal_error
{
public:
    prc_driver_error(std::string const& msg)
        : pdal_error(msg)
    {}
};

PDAL_C_START

PDAL_DLL void PDALRegister_writer_prc(void* factory);

PDAL_C_END

pdal::Writer* createPRCWriter(const pdal::Options& options);

class PDAL_DLL Writer : public pdal::Writer
{
public:
    SET_STAGE_NAME("drivers.prc.writer", "PRC Writer")
    SET_STAGE_ENABLED(true)

    Writer(const Options&);
    ~Writer();

    virtual void initialize();
    virtual void processOptions(const Options& options);
    virtual void ready(PointContext ctx);
    virtual void write(const PointBuffer& pointBuffer);
    virtual void done(PointContext ctx);

    static Options getDefaultOptions();

protected:

    std::unique_ptr<oPRCFile> m_prcFile;
    std::string m_prcFilename;
    pdal::Bounds<double> m_bounds;

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

private:

    Writer& operator=(const Writer&); // not implemented
    Writer(const Writer&); // not implemented

};

}
}
} // namespaces

#endif
