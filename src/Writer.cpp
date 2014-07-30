/******************************************************************************
* This file is part of a tool for producing 3D content in the PRC format.
* Copyright (c) 2013-2014, Bradley J Chambers, brad.chambers@gmail.com
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
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/format.hpp>

#include <hpdf.h>
#include <hpdf_u3d.h>
#include <hpdf_annotation.h>

#include <pdal/Dimension.hpp>
#include <pdal/Schema.hpp>
#include <pdal/PointBuffer.hpp>
#include <pdal/pdal_macros.hpp>
#include <pdal/StageFactory.hpp>

#include <prc/oPRCFile.hpp>
#include <prc/ColorQuantizer.hpp>

MAKE_WRITER_CREATOR(prcWriter, pdal::drivers::prc::Writer)
CREATE_WRITER_PLUGIN(prc, pdal::drivers::prc::Writer)

SET_PLUGIN_VERSION(prc)

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

namespace pdal
{
namespace drivers
{
namespace prc
{

Writer::Writer(const Options& options)
    : pdal::Writer(options)
    , m_outputFormat(OUTPUT_FORMAT_PDF)
    , m_colorScheme(COLOR_SCHEME_SOLID)
    , m_contrastStretch(CONTRAST_STRETCH_LINEAR)
{
    return;
}


Writer::~Writer()
{
    return;
}

void Writer::processOptions(const Options& options)
{
    m_prcFilename = options.getValueOrThrow<std::string>("prc_filename");
    std::string output_format = options.getValueOrDefault<std::string>("output_format", "pdf");
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

    std::string color_scheme = options.getValueOrDefault<std::string>("color_scheme", "solid");
    log()->get(logDEBUG) << color_scheme << " scheme" << std::endl;

    if (boost::iequals(color_scheme, "solid"))
        m_colorScheme = COLOR_SCHEME_SOLID;
    else if (boost::iequals(color_scheme, "oranges"))
        m_colorScheme = COLOR_SCHEME_ORANGES;
    else if (boost::iequals(color_scheme, "blue_green"))
        m_colorScheme = COLOR_SCHEME_BLUE_GREEN;
    else
    {
        std::ostringstream oss;
        oss << "Unrecognized color scheme " << color_scheme;
        throw prc_driver_error("Unrecognized color scheme");
    }

    std::string contrast_stretch = getOptions().getValueOrDefault<std::string>("contrast_stretch", "linear");
    log()->get(logDEBUG) << contrast_stretch << " stretch" << std::endl;

    if (boost::iequals(contrast_stretch, "linear"))
        m_contrastStretch = CONTRAST_STRETCH_LINEAR;
    else if (boost::iequals(contrast_stretch, "sqrt"))
        m_contrastStretch = CONTRAST_STRETCH_SQRT;
    else
    {
        std::ostringstream oss;
        oss << "Unrecognized contrast stretch " << contrast_stretch;
        throw prc_driver_error("Unrecognized contrast stretch");
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

}

void Writer::initialize()
{
    m_prcFile = std::unique_ptr<oPRCFile>(new oPRCFile(m_prcFilename,1000));
}


Options Writer::getDefaultOptions()
{
    Options options;

    Option prc_filename("prc_filename", "", "Filename to write PRC file to");
    Option pdf_filename("pdf_filename", "", "Filename to write PDF file to");
    Option output_format("output_format", "", "PRC or PDF");
    Option color_scheme("color_scheme", "", "Solid, oranges, or blue-green");
    Option contrast_stretch("contrast_stretch", "", "Linear or sqrt");
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
    options.add(color_scheme);
    options.add(contrast_stretch);
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


void Writer::ready(PointContext ctx)
{
    PRCoptions grpopt;
    grpopt.no_break = true;
    grpopt.do_break = false;
    grpopt.tess = true;

    m_prcFile->begingroup("points",&grpopt);

}


void Writer::done(PointContext ctx)

{
    m_prcFile->endgroup();
    m_prcFile->finish();

    if (m_outputFormat == OUTPUT_FORMAT_PDF)
    {
        const float width = 256.0f;
        const float height = 256.0f;
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
            throw pdal_error("Cannot create PdfDoc object!");

        }
        pdf->pdf_version = HPDF_VER_17;

        page = HPDF_AddPage(pdf);
        HPDF_Page_SetWidth(page, width);
        HPDF_Page_SetHeight(page, height);

        std::string prcFilename = getOptions().getValueOrThrow<std::string>("prc_filename");
        log()->get(logDEBUG) << "prcFilename: " << prcFilename << std::endl;

        u3d = HPDF_LoadU3DFromFile(pdf, prcFilename.c_str());
        if (!u3d)
        {
            throw pdal_error("cannot load U3D object!");
        }

        view = HPDF_Create3DView(u3d->mmgr, "DefaultView");
        if (!view)
        {
            throw pdal_error("cannot create DefaultView!");
        }

        log()->get(logDEBUG) << boost::format("camera %f %f %f %f %f %f %f %f") % m_coox % m_cooy % m_cooz % m_c2cx % m_c2cy % m_c2cz % m_roo % m_roll << std::endl ;

        HPDF_3DView_SetCamera(view, m_coox, m_cooy, m_cooz, m_c2cx, m_c2cy, m_c2cz, m_roo, m_roll);
        HPDF_3DView_SetPerspectiveProjection(view, 30.0);
        HPDF_3DView_SetBackgroundColor(view, 0, 0, 0);
        HPDF_3DView_SetLighting(view, "Headlamp");

        HPDF_U3D_Add3DView(u3d, view);
        HPDF_U3D_SetDefault3DView(u3d, "DefaultView");

        // libharu master changes things slightly
        //annot = HPDF_Page_Create3DAnnot(page, rect, false, false, u3d, NULL);
        // but the libharu-dev package expects this
        annot = HPDF_Page_Create3DAnnot(page, rect, u3d);
        if (!annot)
        {
            throw pdal_error("cannot create annotation!");
        }

        //HPDF_Dict action = (HPDF_Dict) HPDF_Dict_GetItem( annot, "3DA", HPDF_OCLASS_DICT );
        //HPDF_Dict_AddBoolean( action, "TB", HPDF_TRUE );

        std::string pdfFilename = getOptions().getValueOrThrow<std::string>("pdf_filename");
        HPDF_STATUS success = HPDF_SaveToFile(pdf, pdfFilename.c_str());

        HPDF_Free(pdf);
    }

    return;
}

void Writer::write(const PointBuffer& data)
{
    uint32_t numPoints = 0;

    m_bounds = data.calculateBounds();
    double zmin = m_bounds.getMinimum(2);
    double zmax = m_bounds.getMaximum(2);
    double cz2 = (zmax-zmin)/2+zmin;
    HPDF_REAL cooz = static_cast<HPDF_REAL>(cz2);

    log()->get(logDEBUG) << boost::format("cz: %f, min: %f, max: %f, cooz: %f") % cz2 % zmin % zmax % cooz << std::endl ;

    double cx = (m_bounds.getMaximum(0)-m_bounds.getMinimum(0))/2+m_bounds.getMinimum(0);
    double cy = (m_bounds.getMaximum(1)-m_bounds.getMinimum(1))/2+m_bounds.getMinimum(1);
    double cz = (m_bounds.getMaximum(2)-m_bounds.getMinimum(2))/2+m_bounds.getMinimum(2);

    pdal::Schema const& schema = data.getSchema();

    pdal::Dimension const& dimX = schema.getDimension("X");
    pdal::Dimension const& dimY = schema.getDimension("Y");
    pdal::Dimension const& dimZ = schema.getDimension("Z");

    if ((m_colorScheme == COLOR_SCHEME_ORANGES) || (m_colorScheme == COLOR_SCHEME_BLUE_GREEN))
    {
        double **p0, **p1, **p2, **p3, **p4, **p5, **p6, **p7, **p8;
        p0 = (double**) malloc(data.size()*sizeof(double*));
        p1 = (double**) malloc(data.size()*sizeof(double*));
        p2 = (double**) malloc(data.size()*sizeof(double*));
        p3 = (double**) malloc(data.size()*sizeof(double*));
        p4 = (double**) malloc(data.size()*sizeof(double*));
        p5 = (double**) malloc(data.size()*sizeof(double*));
        p6 = (double**) malloc(data.size()*sizeof(double*));
        p7 = (double**) malloc(data.size()*sizeof(double*));
        p8 = (double**) malloc(data.size()*sizeof(double*));

        for (point_count_t i = 0; i < data.size(); ++i)
        {
            p0[i] = (double*) malloc(3*sizeof(double));
            p1[i] = (double*) malloc(3*sizeof(double));
            p2[i] = (double*) malloc(3*sizeof(double));
            p3[i] = (double*) malloc(3*sizeof(double));
            p4[i] = (double*) malloc(3*sizeof(double));
            p5[i] = (double*) malloc(3*sizeof(double));
            p6[i] = (double*) malloc(3*sizeof(double));
            p7[i] = (double*) malloc(3*sizeof(double));
            p8[i] = (double*) malloc(3*sizeof(double));
        }

        double xd(0.0);
        double yd(0.0);
        double zd(0.0);

        RGBAColour c0, c1, c2, c3, c4, c5, c6, c7, c8;

        if (m_colorScheme == COLOR_SCHEME_BLUE_GREEN)
        {
            c8.Set(247.0/255.0, 252.0/255.0, 253.0/255.0);
            c7.Set(229.0/255.0, 245.0/255.0, 249.0/255.0);
            c6.Set(204.0/255.0, 236.0/255.0, 230.0/255.0);
            c5.Set(153.0/255.0, 216.0/255.0, 201.0/255.0);
            c4.Set(102.0/255.0, 194.0/255.0, 164.0/255.0);
            c3.Set(65.0/255.0, 174.0/255.0, 118.0/255.0);
            c2.Set(35.0/255.0, 139.0/255.0,  69.0/255.0);
            c1.Set(0.0, 109.0/255.0,  44.0/255.0);
            c0.Set(0.0,  68.0/255.0,  27.0/255.0);
        }
        else if (m_colorScheme == COLOR_SCHEME_ORANGES)
        {
            c8.Set(255.0/255.0, 245.0/255.0, 235.0/255.0);
            c7.Set(254.0/255.0, 230.0/255.0, 206.0/255.0);
            c6.Set(253.0/255.0, 208.0/255.0, 162.0/255.0);
            c5.Set(253.0/255.0, 174.0/255.0, 107.0/255.0);
            c4.Set(253.0/255.0, 141.0/255.0,  60.0/255.0);
            c3.Set(241.0/255.0, 105.0/255.0,  19.0/255.0);
            c2.Set(217.0/255.0,  72.0/255.0,   1.0/255.0);
            c1.Set(166.0/255.0,  54.0/255.0,   3.0/255.0);
            c0.Set(127.0/255.0,  39.0/255.0,   4.0/255.0);
        }

        double range(0.0), step(0.0), t0(0.0), t1(0.0), t2(0.0), t3(0.0), t4(0.0), t5(0.0), t6(0.0), t7(0.0);

        if (m_contrastStretch == CONTRAST_STRETCH_SQRT)
        {
            range = std::sqrt(m_bounds.getMaximum(2)) - std::sqrt(m_bounds.getMinimum(2));
            step = range / 9;
            t0 = std::sqrt(m_bounds.getMinimum(2)) + 1*step;
            t1 = std::sqrt(m_bounds.getMinimum(2)) + 2*step;
            t2 = std::sqrt(m_bounds.getMinimum(2)) + 3*step;
            t3 = std::sqrt(m_bounds.getMinimum(2)) + 4*step;
            t4 = std::sqrt(m_bounds.getMinimum(2)) + 5*step;
            t5 = std::sqrt(m_bounds.getMinimum(2)) + 6*step;
            t6 = std::sqrt(m_bounds.getMinimum(2)) + 7*step;
            t7 = std::sqrt(m_bounds.getMinimum(2)) + 8*step;

            t0 = t0*t0;
            t1 = t1*t1;
            t2 = t2*t2;
            t3 = t3*t3;
            t4 = t4*t4;
            t5 = t5*t5;
            t6 = t6*t6;
            t7 = t7*t7;
        }
        else if (0)
        {
            range = m_bounds.getMaximum(2) - m_bounds.getMinimum(2);
            double twoper = range * 0.02;
            log()->get(logDEBUG) << twoper << std::endl;
            step = (range - 2 * twoper) / 7;
            t0 = m_bounds.getMinimum(2) + twoper;
            t1 = m_bounds.getMinimum(2) + twoper + 1 * step;
            t2 = m_bounds.getMinimum(2) + twoper + 2 * step;
            t3 = m_bounds.getMinimum(2) + twoper + 3 * step;
            t4 = m_bounds.getMinimum(2) + twoper + 4 * step;
            t5 = m_bounds.getMinimum(2) + twoper + 5 * step;
            t6 = m_bounds.getMinimum(2) + twoper + 6 * step;
            t7 = m_bounds.getMinimum(2) + twoper + 7 * step;
        }
        else if (m_contrastStretch == CONTRAST_STRETCH_LINEAR)
        {
            range = m_bounds.getMaximum(2) - m_bounds.getMinimum(2);
            step = range / 9;
            t0 = m_bounds.getMinimum(2) + 1 * step;
            t1 = m_bounds.getMinimum(2) + 2 * step;
            t2 = m_bounds.getMinimum(2) + 3 * step;
            t3 = m_bounds.getMinimum(2) + 4 * step;
            t4 = m_bounds.getMinimum(2) + 5 * step;
            t5 = m_bounds.getMinimum(2) + 6 * step;
            t6 = m_bounds.getMinimum(2) + 7 * step;
            t7 = m_bounds.getMinimum(2) + 8 * step;
        }

        log()->get(logDEBUG) << boost::format("z stats %f, %f, %f, %f, %f, %f, %f, %f, %f, %f") % range % step % t0 % t1 % t2 % t3 % t4 % t5 %t6 % t7 << std::endl ;
        t0 -= cz;
        t1 -= cz;
        t2 -= cz;
        t3 -= cz;
        t4 -= cz;
        t5 -= cz;
        t6 -= cz;
        t7 -= cz;
        log()->get(logDEBUG) << boost::format("z stats %f, %f, %f, %f, %f, %f, %f, %f, %f, %f") % range % step % t0 % t1 % t2 % t3 % t4 % t5 %t6 % t7 << std::endl;

        int id0, id1, id2, id3, id4, id5, id6, id7, id8;
        id0 = id1 = id2 = id3 = id4 = id5 = id6 = id7 = id8 = 0;

        for (point_count_t i = 0; i < data.size(); ++i)
        {
            double dx = data.getFieldAs<double>(dimX, i) - cx;
            double dy = data.getFieldAs<double>(dimY, i) - cy;
            double dz = data.getFieldAs<double>(dimZ, i) - cz;
            //  if (i % 1000 == 0) printf("%f %f %f\n", xd, yd, zd);

            if (zd < t0)
            {
                p0[id0][0] = xd;
                p0[id0][1] = yd;
                p0[id0][2] = zd;
                id0++;
            }
            else if (zd < t1)
            {
                p1[id1][0] = xd;
                p1[id1][1] = yd;
                p1[id1][2] = zd;
                id1++;
            }
            else if (zd < t2)
            {
                p2[id2][0] = xd;
                p2[id2][1] = yd;
                p2[id2][2] = zd;
                id2++;
            }
            else if (zd < t3)
            {
                p3[id3][0] = xd;
                p3[id3][1] = yd;
                p3[id3][2] = zd;
                id3++;
            }
            else if (zd < t4)
            {
                p4[id4][0] = xd;
                p4[id4][1] = yd;
                p4[id4][2] = zd;
                id4++;
            }
            else if (zd < t5)
            {
                p5[id5][0] = xd;
                p5[id5][1] = yd;
                p5[id5][2] = zd;
                id5++;
            }
            else if (zd < t6)
            {
                p6[id6][0] = xd;
                p6[id6][1] = yd;
                p6[id6][2] = zd;
                id6++;
            }
            else if (zd < t7)
            {
                p7[id7][0] = xd;
                p7[id7][1] = yd;
                p7[id7][2] = zd;
                id7++;
            }
            else
            {
                p8[id8][0] = xd;
                p8[id8][1] = yd;
                p8[id8][2] = zd;
                id8++;
            }

            numPoints++;
        }

        log()->get(logDEBUG) << boost::format("ids: %d %d %d %d %d %d %d %d %d")  % id0 % id1 % id2 % id3 % id4 % id5 % id6 % id7 % id8 ;

        m_prcFile->addPoints(id0, const_cast<const double**>(p0), c0, 1.0);
        m_prcFile->addPoints(id1, const_cast<const double**>(p1), c1, 1.0);
        m_prcFile->addPoints(id2, const_cast<const double**>(p2), c2, 1.0);
        m_prcFile->addPoints(id3, const_cast<const double**>(p3), c3, 1.0);
        m_prcFile->addPoints(id4, const_cast<const double**>(p4), c4, 1.0);
        m_prcFile->addPoints(id5, const_cast<const double**>(p5), c5, 1.0);
        m_prcFile->addPoints(id6, const_cast<const double**>(p6), c6, 1.0);
        m_prcFile->addPoints(id7, const_cast<const double**>(p7), c7, 1.0);
        m_prcFile->addPoints(id8, const_cast<const double**>(p8), c8, 1.0);

        for (point_count_t i = 0; i < data.size(); ++i)
        {
            free(p0[i]);
            free(p1[i]);
            free(p2[i]);
            free(p3[i]);
            free(p4[i]);
            free(p5[i]);
            free(p6[i]);
            free(p7[i]);
            free(p8[i]);
        }
        free(p0);
        free(p1);
        free(p2);
        free(p3);
        free(p4);
        free(p5);
        free(p6);
        free(p7);
        free(p8);
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
            uint16_t histogram[INT16_MAX] = {0};

            double xd(0.0);
            double yd(0.0);
            double zd(0.0);

            for (point_count_t point = 0; point < data.size(); ++point)
            {
                uint16_t r = data.getField<uint16_t>(*dimR, point);
                uint16_t g = data.getField<uint16_t>(*dimG, point);
                uint16_t b = data.getField<uint16_t>(*dimB, point);
                uint16_t color = RGB(r, g, b);
                histogram[color]++;
            }

            byte colMap[256][3];
            ColorQuantizer *colorQuantizer = new ColorQuantizer();
            // is there any chance that this won't return 256 cubes? should we check?
            word ncubes = colorQuantizer->medianCut(histogram, colMap, 256);

            std::vector<std::vector<int> > indices;
            indices.resize(256);

            for (point_count_t point = 0; point < data.size(); ++point)
            {
                uint16_t r = data.getField<uint16_t>(*dimR, point);
                uint16_t g = data.getField<uint16_t>(*dimG, point);
                uint16_t b = data.getField<uint16_t>(*dimB, point);
                uint16_t color = RGB(r, g, b);
                uint16_t colorIndex = histogram[color];
                indices[colorIndex].push_back(point);
            }

            for (uint32_t level = 0; level < 256; ++level)
            {
                int num_points = indices[level].size();

                double **points;

                // Allocate memory
                points = new double*[num_points];
                for (int point = 0; point < num_points; ++point)
                {
                    points[point] = new double[3];
                    
                    int idx = indices[level][point];

                    xd = data.getFieldAs<double>(dimX, idx) - cx;
                    yd = data.getFieldAs<double>(dimY, idx) - cy;
                    zd = data.getFieldAs<double>(dimZ, idx) - cz;

                    points[point][0] = xd;
                    points[point][1] = yd;
                    points[point][2] = zd;
                    numPoints++;
                }

                double r = static_cast<double>((int)(colMap[level][0])/255.0);
                double g = static_cast<double>((int)(colMap[level][1])/255.0);
                double b = static_cast<double>((int)(colMap[level][2])/255.0);

                m_prcFile->addPoints(num_points, const_cast<const double**>(points), RGBAColour(r, g, b, 1.0), 5.0);

                // De-Allocate memory to prevent memory leak
                for (int point = 0; point < num_points; ++point)
                {
                    delete [] points[point];
                }
                delete [] points;
            }
        }
        else
        {
            double **points;
            points = (double**) malloc(data.size()*sizeof(double*));
            for (point_count_t i = 0; i < data.size(); ++i)
            {
                points[i] = (double*) malloc(3*sizeof(double));
            }

            double xd(0.0);
            double yd(0.0);
            double zd(0.0);

            for (point_count_t i = 0; i < data.size(); ++i)
            {
                xd = data.getFieldAs<double>(dimX, i) - cx;
                yd = data.getFieldAs<double>(dimY, i) - cy;
                zd = data.getFieldAs<double>(dimZ, i) - cz;

                if (i % 10000 == 0) 
                {
                    log()->get(logDEBUG) << boost::format("small point %f %f %f")  % xd % yd % zd ;
                }
                points[i][0] = xd;
                points[i][1] = yd;
                points[i][2] = zd;

                numPoints++;
            }

            m_prcFile->addPoints(numPoints, const_cast<const double**>(points), RGBAColour(1.0,1.0,0.0,1.0),1.0);

            for (point_count_t i = 0; i < data.size(); ++i)
            {
                free(points[i]);
            }
            free(points);
        }
    }

}

}
}
} // namespaces
