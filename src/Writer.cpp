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

#include <prc/Writer.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/erase.hpp>

#include <hpdf.h>
#include <hpdf_u3d.h>
#include <hpdf_annotation.h>

#include <pdal/Dimension.hpp>
#include <pdal/Schema.hpp>
#include <pdal/PointBuffer.hpp>
#include <pdal/pdal_macros.hpp>
#include <pdal/StageFactory.hpp>

#include <prc/oPRCFile.hpp>

MAKE_WRITER_CREATOR(prcWriter, pdal::drivers::prc::Writer)
CREATE_WRITER_PLUGIN(prc, pdal::drivers::prc::Writer)

enum OUTPUT_FORMAT
{
    OUTPUT_FORMAT_PDF,
    OUTPUT_FORMAT_PRC
};

enum COLOR_SCALE
{
    COLOR_SCALE_NONE,
    COLOR_SCALE_AUTO
};


namespace pdal
{
namespace drivers
{
namespace prc
{

Writer::Writer(Stage& prevStage, const Options& options)
    : pdal::Writer(prevStage, options)
    , m_prcFile(options.getOption("prc_filename").getValue<std::string>())
    , m_outputFormat(OUTPUT_FORMAT_PDF)
    , m_colorScale(COLOR_SCALE_NONE)
{
    std::cout << options.getOption("prc_filename").getValue<std::string>() << std::endl;

    m_bounds = prevStage.getBounds();
    double zmin = m_bounds.getMinimum(2);
    double zmax = m_bounds.getMaximum(2);
    double cz = (zmax-zmin)/2+zmin;
    HPDF_REAL cooz = static_cast<HPDF_REAL>(cz);

    printf("cz: %f, min: %f, max: %f, cooz: %f\n", cz, zmin, zmax, cooz);

    return;
}


Writer::~Writer()
{
    return;
}


void Writer::initialize()
{
    pdal::Writer::initialize();

    std::string output_format = getOptions().getValueOrDefault<std::string>("output_format", "pdf");

    if (boost::iequals(output_format, "pdf"))
        m_outputFormat = OUTPUT_FORMAT_PDF;
    else if (boost::iequals(output_format, "prc"))
        m_outputFormat = OUTPUT_FORMAT_PRC;
    else
    {
        std::ostringstream oss;
        oss << "Unrecognized output format " << output_format;
        throw prc_driver_error("Unrecognized output format");
    }

    std::string color_scale = getOptions().getValueOrDefault<std::string>("color_scale", "none");

    if (boost::iequals(color_scale, "none"))
        m_colorScale = COLOR_SCALE_NONE;
    else if (boost::iequals(color_scale, "auto"))
        m_colorScale = COLOR_SCALE_AUTO;
    else
    {
        std::ostringstream oss;
        oss << "Unrecognized color scale " << color_scale;
        throw prc_driver_error("Unrecognized color scale");
    }

    m_fov = static_cast<HPDF_REAL>(getOptions().getValueOrDefault<double>("fov", 30.0f));
    m_coox = static_cast<HPDF_REAL>(getOptions().getValueOrDefault<double>("coox", 0.0f));
    m_cooy = static_cast<HPDF_REAL>(getOptions().getValueOrDefault<double>("cooy", 0.0f));
    m_cooz = static_cast<HPDF_REAL>(getOptions().getValueOrDefault<double>("cooz", 0.0f));
    m_c2cx = static_cast<HPDF_REAL>(getOptions().getValueOrDefault<double>("c2cx", 0.0f));
    m_c2cy = static_cast<HPDF_REAL>(getOptions().getValueOrDefault<double>("c2cy", 0.0f));
    m_c2cz = static_cast<HPDF_REAL>(getOptions().getValueOrDefault<double>("c2cz", 1.0f));
    m_roo = static_cast<HPDF_REAL>(getOptions().getValueOrDefault<double>("roo", 20.0f));
    m_roll = static_cast<HPDF_REAL>(getOptions().getValueOrDefault<double>("roll", 0.0f));

    return;
}


Options Writer::getDefaultOptions()
{
    Options options;

    Option prc_filename("prc_filename", "", "Filename to write PRC file to");
    Option pdf_filename("pdf_filename", "", "Filename to write PDF file to");
    Option output_format("output_format", "", "PRC or PDF");
    Option color_scale("color_scale", "", "None or auto");
    Option fov("fov", "", "Field of View");
    Option coox("coox", "", "Camera coox");
    Option cooy("cooy", "", "Camera cooy");
    Option cooz("cooz", "", "Camera cooz");
    Option c2cx("c2cx", "", "Camera c2cx");
    Option c2cy("c2cy", "", "Camera c2cy");
    Option c2cz("c2cz", "", "Camera c2cz");
    Option roo("roo", "", "Camera roo");
    Option roll("roll", "", "Camera roll");

    options.add(prc_filename);
    options.add(pdf_filename);
    options.add(output_format);
    options.add(color_scale);
    options.add(fov);
    options.add(coox);
    options.add(cooy);
    options.add(cooz);
    options.add(c2cx);
    options.add(c2cy);
    options.add(c2cz);
    options.add(roo);
    options.add(roll);

    return options;
}


void Writer::writeBegin(boost::uint64_t /*targetNumPointsToWrite*/)
{
    PRCoptions grpopt;
    grpopt.no_break = true;
    grpopt.do_break = false;
    grpopt.tess = true;

    m_prcFile.begingroup("points",&grpopt);

    return;
}


void Writer::writeEnd(boost::uint64_t /*actualNumPointsWritten*/)
{
    m_prcFile.endgroup();
    m_prcFile.finish();

    if (m_outputFormat == OUTPUT_FORMAT_PDF)
    {
        const double width = 256.0f;
        const double height = 256.0f;
        const double depth = std::sqrt(width*height);

        const HPDF_Rect rect = { 0, 0, width, height };
        HPDF_Doc pdf;
        HPDF_Page page;
        HPDF_Annotation annot;
        HPDF_U3D u3d;
        HPDF_Dict view;

        pdf = HPDF_New(NULL, NULL);
        if (!pdf)
        {
            printf("error: cannot create PdfDoc object\n");
            return;
        }
        pdf->pdf_version = HPDF_VER_17;

        page = HPDF_AddPage(pdf);
        HPDF_Page_SetWidth(page, width);
        HPDF_Page_SetHeight(page, height);

        std::string prcFilename = getOptions().getValueOrThrow<std::string>("prc_filename");
        u3d = HPDF_LoadU3DFromFile(pdf, prcFilename.c_str());
        if (!u3d)
        {
            printf("error: cannot load U3D object\n");
            return;
        }

        view = HPDF_Create3DView(u3d->mmgr, "DefaultView");
        if (!view)
        {
            printf("error: cannot create view\n");
            return;
        }

        printf("%f %f %f %f %f %f %f %f\n", m_coox, m_cooy, m_cooz, m_c2cx, m_c2cy, m_c2cz, m_roo, m_roll);

        HPDF_3DView_SetCamera(view, m_coox, m_cooy, m_cooz, m_c2cx, m_c2cy, m_c2cz, m_roo, m_roll);
        HPDF_3DView_SetPerspectiveProjection(view, 30.0);
        HPDF_3DView_SetBackgroundColor(view, 0, 0, 0);
        HPDF_3DView_SetLighting(view, "Headlamp");

        HPDF_U3D_Add3DView(u3d, view);
        HPDF_U3D_SetDefault3DView(u3d, "DefaultView");

        annot = HPDF_Page_Create3DAnnot(page, rect, u3d);
        if (!annot)
        {
            printf("error: cannot create annotation\n");
            return;
        }

        //HPDF_Dict action = (HPDF_Dict) HPDF_Dict_GetItem( annot, "3DA", HPDF_OCLASS_DICT );
        //HPDF_Dict_AddBoolean( action, "TB", HPDF_TRUE );

        std::string pdfFilename = getOptions().getValueOrThrow<std::string>("pdf_filename");
        HPDF_STATUS success = HPDF_SaveToFile(pdf, pdfFilename.c_str());

        HPDF_Free(pdf);
    }

    return;
}

boost::uint32_t Writer::writeBuffer(const PointBuffer& data)
{
    boost::uint32_t numPoints = 0;

    double cx = (m_bounds.getMaximum(0)-m_bounds.getMinimum(0))/2+m_bounds.getMinimum(0);
    double cy = (m_bounds.getMaximum(1)-m_bounds.getMinimum(1))/2+m_bounds.getMinimum(1);
    double cz = (m_bounds.getMaximum(2)-m_bounds.getMinimum(2))/2+m_bounds.getMinimum(2);

    pdal::Schema const& schema = data.getSchema();

    pdal::Dimension const& dimX = schema.getDimension("X");
    pdal::Dimension const& dimY = schema.getDimension("Y");
    pdal::Dimension const& dimZ = schema.getDimension("Z");

    if (m_colorScale == COLOR_SCALE_AUTO)
    {
        double **points;
        points = (double**) malloc(sizeof(double*));
        {
            points[0] = (double*) malloc(3*sizeof(double));
        }

        double xd(0.0);
        double yd(0.0);
        double zd(0.0);

        for (boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
        {
            if (dimX.getByteSize() == 4 && dimX.getInterpretation() == pdal::dimension::Float)
            {
                for (boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
                {
                    float x = data.getField<float>(dimX, i);
                    float y = data.getField<float>(dimY, i);
                    float z = data.getField<float>(dimZ, i);

                    xd = dimX.applyScaling<float>(x) - cx;
                    yd = dimY.applyScaling<float>(y) - cy;
                    zd = dimZ.applyScaling<float>(z) - cz;

                    points[i][0] = xd;
                    points[i][1] = yd;
                    points[i][2] = zd;

                    double r = (dimZ.applyScaling<float>(z) - m_bounds.getMinimum(2)) / (m_bounds.getMaximum(2) - m_bounds.getMinimum(2));
                    double g, b;
                    g = b = 0.0f;
                    if (i % 1000 == 0) printf("%f %f %f %f %f %f\n", xd, yd, zd, r, g, b);

                    m_prcFile.addPoints(1, const_cast<const double**>(points), RGBAColour(r,0.0,0.0,1.0), 5.0);

                    numPoints++;
                }
            }
            else if (dimX.getByteSize() == 4 && dimX.getInterpretation() == pdal::dimension::SignedInteger)
            {
                for (boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
                {
                    boost::int32_t x = data.getField<boost::int32_t>(dimX, i);
                    boost::int32_t y = data.getField<boost::int32_t>(dimY, i);
                    boost::int32_t z = data.getField<boost::int32_t>(dimZ, i);

                    xd = dimX.applyScaling<boost::int32_t>(x) - cx;
                    yd = dimY.applyScaling<boost::int32_t>(y) - cy;
                    zd = dimZ.applyScaling<boost::int32_t>(z) - cz;

                    if (i % 10000) printf("%f %f %f\n", xd, yd, zd);

                    points[i][0] = xd;
                    points[i][1] = yd;
                    points[i][2] = zd;

                    double r = (dimZ.applyScaling<boost::int32_t>(z) - m_bounds.getMinimum(2)) / (m_bounds.getMaximum(2) - m_bounds.getMinimum(2));
                    double g, b;
                    g = b = 0.0f;
                    if (i % 1000 == 0) printf("%f %f %f %f %f %f\n", xd, yd, zd, r, g, b);

                    m_prcFile.addPoints(1, const_cast<const double**>(points), RGBAColour(r,0.0,0.0,1.0), 5.0);

                    numPoints++;
                }
            }
            else
            {
                std::cerr << "didn't detect a suitable dimension interpretation\n";
            }
        }

        {
            free(points[0]);
        }
        free(points);
    }
    else
    {
        boost::optional<pdal::Dimension const&> dimR = schema.getDimensionOptional("Red");
        boost::optional<pdal::Dimension const&> dimG = schema.getDimensionOptional("Green");
        boost::optional<pdal::Dimension const&> dimB = schema.getDimensionOptional("Blue");

        bool bHaveColor(false);
        if (dimR && dimG && dimB)
            bHaveColor = true;

        if (bHaveColor)
        {
            double **points;
            points = (double**) malloc(sizeof(double*));
            {
                points[0] = (double*) malloc(3*sizeof(double));
            }

            double xd(0.0);
            double yd(0.0);
            double zd(0.0);

            for (boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
            {
                boost::int32_t x = data.getField<boost::int32_t>(dimX, i);
                boost::int32_t y = data.getField<boost::int32_t>(dimY, i);
                boost::int32_t z = data.getField<boost::int32_t>(dimZ, i);

                double r = static_cast<double>(data.getField<boost::uint16_t>(*dimR, i))/255.0;
                double g = static_cast<double>(data.getField<boost::uint16_t>(*dimG, i))/255.0;
                double b = static_cast<double>(data.getField<boost::uint16_t>(*dimB, i))/255.0;

                xd = dimX.applyScaling<boost::int32_t>(x) - cx;
                yd = dimY.applyScaling<boost::int32_t>(y) - cy;
                zd = dimZ.applyScaling<boost::int32_t>(z) - cz;

                if (i % 1000 == 0) printf("%f %f %f %f %f %f\n", xd, yd, zd, r, g, b);

                points[0][0] = xd;
                points[0][1] = yd;
                points[0][2] = zd;

                m_prcFile.addPoints(1, const_cast<const double**>(points), RGBAColour(r,g,b,1.0), 5.0);

                numPoints++;
            }

            {
                free(points[0]);
            }
            free(points);
        }
        else
        {
            double **points;
            points = (double**) malloc(data.getNumPoints()*sizeof(double*));
            for (boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
            {
                points[i] = (double*) malloc(3*sizeof(double));
            }

            double xd(0.0);
            double yd(0.0);
            double zd(0.0);

            if (dimX.getByteSize() == 4 && dimX.getInterpretation() == pdal::dimension::Float)
            {
                for (boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
                {
                    float x = data.getField<float>(dimX, i);
                    float y = data.getField<float>(dimY, i);
                    float z = data.getField<float>(dimZ, i);

                    xd = dimX.applyScaling<float>(x) - cx;
                    yd = dimY.applyScaling<float>(y) - cy;
                    zd = dimZ.applyScaling<float>(z) - cz;

                    points[i][0] = xd;
                    points[i][1] = yd;
                    points[i][2] = zd;

                    numPoints++;
                }
            }
            else if (dimX.getByteSize() == 4 && dimX.getInterpretation() == pdal::dimension::SignedInteger)
            {
                for (boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
                {
                    boost::int32_t x = data.getField<boost::int32_t>(dimX, i);
                    boost::int32_t y = data.getField<boost::int32_t>(dimY, i);
                    boost::int32_t z = data.getField<boost::int32_t>(dimZ, i);

                    xd = dimX.applyScaling<boost::int32_t>(x) - cx;
                    yd = dimY.applyScaling<boost::int32_t>(y) - cy;
                    zd = dimZ.applyScaling<boost::int32_t>(z) - cz;

                    if (i % 10000) printf("%f %f %f\n", xd, yd, zd);

                    points[i][0] = xd;
                    points[i][1] = yd;
                    points[i][2] = zd;

                    numPoints++;
                }
            }
            else
            {
                std::cerr << "didn't detect a suitable dimension interpretation\n";
            }

            m_prcFile.addPoints(numPoints, const_cast<const double**>(points), RGBAColour(1.0,1.0,0.0,1.0),1.0);

            for (boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
            {
                free(points[i]);
            }
            free(points);
        }
    }

    return numPoints;
}

}
}
} // namespaces
