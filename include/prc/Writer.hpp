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

pdal::Writer* createPRCWriter(pdal::Stage& prevStage, const pdal::Options& options);

class PDAL_DLL Writer : public pdal::Writer
{
public:
    SET_STAGE_NAME("drivers.prc.writer", "PRC Writer")

    Writer(Stage& prevStage, const Options&);
    ~Writer();

    virtual void initialize();
    static Options getDefaultOptions();

protected:
    virtual void writeBegin(boost::uint64_t targetNumPointsToWrite);
    virtual boost::uint32_t writeBuffer(const PointBuffer&);
    virtual void writeEnd(boost::uint64_t actualNumPointsWritten);

    oPRCFile m_prcFile;

    pdal::Bounds<double> m_bounds;

    int m_outputFormat;
    int m_colorScale;

    double m_fov;
    double m_heading;

    HPDF_REAL m_coox, m_cooy, m_cooz;

    void setCOOx(HPDF_REAL coox)
    {
        m_coox = coox;
    }
    void setCOOy(HPDF_REAL cooy)
    {
        m_cooy = cooy;
    }
    void setCOOz(HPDF_REAL cooz)
    {
        m_cooz = cooz;
    }

    HPDF_REAL getCOOx()
    {
        return m_coox;
    }
    HPDF_REAL getCOOy()
    {
        return m_cooy;
    }
    HPDF_REAL getCOOz()
    {
        return m_cooz;
    }

private:

    Writer& operator=(const Writer&); // not implemented
    Writer(const Writer&); // not implemented

};

}
}
} // namespaces

#endif
