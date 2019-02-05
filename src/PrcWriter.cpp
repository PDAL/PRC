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

#include "PrcWriter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <hpdf.h>
#include <hpdf_u3d.h>
#include <hpdf_annotation.h>

#include <pdal/Dimension.hpp>
#include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/util/Utils.hpp>

#include "oPRCFile.hpp"
#include "ColorQuantizer.hpp"

namespace pdal
{

static PluginInfo const s_info = PluginInfo(
                                     "writers.prc",
                                     "PRC Writer",
                                     "");

CREATE_SHARED_PLUGIN(1, 0, PrcWriter, Writer, s_info)

std::string PrcWriter::getName() const
{
    return s_info.name;
}


void PrcWriter::addArgs(ProgramArgs& args)
{
    args.add("filename", "Filename to write PRC file to",
        m_prcFilename).setPositional();
    args.addSynonym("filename", "prc_filename");
    args.add("pdf_filename", "Filename to write PDF file to",
        m_pdfFilename).setPositional();
    args.add("output_format", "PRC or PDF", m_outputFormat, OutputFormat::Pdf);
    args.add("color_scheme", "Solid, oranges, or blue-green",
        m_colorScheme, ColorScheme::Solid);
    args.add("contrast_stretch", "Linear or sqrt", m_contrastStretch,
        ContrastStretch::Linear);
    args.add("fov", "Field of View", m_fov, 30.0f);
    args.add("coox", "Camera coox", m_coox);
    args.add("cooy", "Camera cooy", m_cooy);
    args.add("cooz", "Camera cooz", m_cooz);
    args.add("c2cx", "Camera c2cx", m_c2cx);
    args.add("c2cy", "Camera c2cy", m_c2cy);
    args.add("c2cz", "Camera c2cz", m_c2cz);
    args.add("roo", "Camera roo", m_roo, 20.0f);
    args.add("roll", "Camera roll", m_roll);
}


void PrcWriter::initialize()
{
    m_prcFile = std::unique_ptr<oPRCFile>(new oPRCFile(m_prcFilename,1000));
}


void PrcWriter::ready(PointTableRef table)
{
    PRCoptions grpopt;
    grpopt.no_break = true;
    grpopt.do_break = false;
    grpopt.tess = true;

    m_prcFile->begingroup("points",&grpopt);
}


void PrcWriter::done(PointTableRef table)
{
    log()->get(LogLevel::Debug4) << "Finalizing PRC." << std::endl;
    m_prcFile->endgroup();
    m_prcFile->finish();

    if (m_outputFormat == OutputFormat::Pdf)
    {
        log()->get(LogLevel::Debug4) << "Writing PDF." << std::endl;

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

        log()->get(LogLevel::Debug2) << "prcFilename: " <<
            m_prcFilename << std::endl;

        u3d = HPDF_LoadU3DFromFile(pdf, m_prcFilename.c_str());
        if (!u3d)
        {
            throw pdal_error("cannot load U3D object!");
        }

        view = HPDF_Create3DView(u3d->mmgr, "DefaultView");
        if (!view)
        {
            throw pdal_error("cannot create DefaultView!");
        }

        char msg[1000];
        sprintf(msg, "camera %f %f %f %f %f %f %f %f", m_coox,
                m_cooy, m_cooz, m_c2cx, m_c2cy, m_c2cz, m_roo, m_roll);
        log()->get(LogLevel::Debug2) << msg << std::endl ;

        HPDF_3DView_SetCamera(view, m_coox, m_cooy, m_cooz, m_c2cx, m_c2cy,
                              m_c2cz, m_roo, m_roll);
        HPDF_3DView_SetPerspectiveProjection(view, 30.0);
        HPDF_3DView_SetBackgroundColor(view, 0, 0, 0);
        HPDF_3DView_SetLighting(view, "Headlamp");

        HPDF_U3D_Add3DView(u3d, view);
        HPDF_U3D_SetDefault3DView(u3d, "DefaultView");

        // libharu master changes things slightly
        annot = HPDF_Page_Create3DAnnot(page, rect, false, false, u3d, NULL);
        if (!annot)
        {
            throw pdal_error("cannot create annotation!");
        }

        //HPDF_Dict action = (HPDF_Dict) HPDF_Dict_GetItem( annot, "3DA", HPDF_OCLASS_DICT );
        //HPDF_Dict_AddBoolean( action, "TB", HPDF_TRUE );

        HPDF_SaveToFile(pdf, m_pdfFilename.c_str());
        HPDF_Free(pdf);
    }
}

void PrcWriter::write(const PointViewPtr view)
{
    uint32_t numPoints = 0;

    view->calculateBounds(m_bounds);
    double zmin = m_bounds.minz;
    double zmax = m_bounds.maxz;
    double cz2 = (zmax-zmin)/2+zmin;
    HPDF_REAL cooz = static_cast<HPDF_REAL>(cz2);

    char msg[1000];
    sprintf(msg, "cz: %f, min: %f, max: %f, cooz: %f", cz2, zmin, zmax, cooz);
    log()->get(LogLevel::Debug2) << msg << std::endl ;

    double cx = (m_bounds.maxx-m_bounds.minx)/2+m_bounds.minx;
    double cy = (m_bounds.maxy-m_bounds.miny)/2+m_bounds.miny;
    double cz = (m_bounds.maxz-m_bounds.minz)/2+m_bounds.minz;

    if ((m_colorScheme == ColorScheme::Oranges) ||
            (m_colorScheme == ColorScheme::BlueGreen))
    {
        double **p0, **p1, **p2, **p3, **p4, **p5, **p6, **p7, **p8;
        p0 = (double**) malloc(view->size()*sizeof(double*));
        p1 = (double**) malloc(view->size()*sizeof(double*));
        p2 = (double**) malloc(view->size()*sizeof(double*));
        p3 = (double**) malloc(view->size()*sizeof(double*));
        p4 = (double**) malloc(view->size()*sizeof(double*));
        p5 = (double**) malloc(view->size()*sizeof(double*));
        p6 = (double**) malloc(view->size()*sizeof(double*));
        p7 = (double**) malloc(view->size()*sizeof(double*));
        p8 = (double**) malloc(view->size()*sizeof(double*));

        for (point_count_t i = 0; i < view->size(); ++i)
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

        if (m_colorScheme == ColorScheme::BlueGreen)
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
        else if (m_colorScheme == ColorScheme::Oranges)
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

        double range(0.0), step(0.0), t0(0.0), t1(0.0), t2(0.0), t3(0.0),
               t4(0.0), t5(0.0), t6(0.0), t7(0.0);

        if (m_contrastStretch == ContrastStretch::Sqrt)
        {
            range = std::sqrt(m_bounds.maxz) - std::sqrt(m_bounds.minz);
            step = range / 9;
            t0 = std::sqrt(m_bounds.minz) + 1*step;
            t1 = std::sqrt(m_bounds.minz) + 2*step;
            t2 = std::sqrt(m_bounds.minz) + 3*step;
            t3 = std::sqrt(m_bounds.minz) + 4*step;
            t4 = std::sqrt(m_bounds.minz) + 5*step;
            t5 = std::sqrt(m_bounds.minz) + 6*step;
            t6 = std::sqrt(m_bounds.minz) + 7*step;
            t7 = std::sqrt(m_bounds.minz) + 8*step;

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
            range = m_bounds.minz - m_bounds.minz;
            double twoper = range * 0.02;
            log()->get(LogLevel::Debug2) << twoper << std::endl;
            step = (range - 2 * twoper) / 7;
            t0 = m_bounds.minz + twoper;
            t1 = m_bounds.minz + twoper + 1 * step;
            t2 = m_bounds.minz + twoper + 2 * step;
            t3 = m_bounds.minz + twoper + 3 * step;
            t4 = m_bounds.minz + twoper + 4 * step;
            t5 = m_bounds.minz + twoper + 5 * step;
            t6 = m_bounds.minz + twoper + 6 * step;
            t7 = m_bounds.minz + twoper + 7 * step;
        }
        else if (m_contrastStretch == ContrastStretch::Linear)
        {
            range = m_bounds.maxz - m_bounds.minz;
            step = range / 9;
            t0 = m_bounds.minz + 1 * step;
            t1 = m_bounds.minz + 2 * step;
            t2 = m_bounds.minz + 3 * step;
            t3 = m_bounds.minz + 4 * step;
            t4 = m_bounds.minz + 5 * step;
            t5 = m_bounds.minz + 6 * step;
            t6 = m_bounds.minz + 7 * step;
            t7 = m_bounds.minz + 8 * step;
        }

        char msg[1000];
        sprintf(msg, "z stats %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
                range, step, t0, t1, t2, t3, t4, t5, t6, t7);
        log()->get(LogLevel::Debug2) << msg << std::endl ;
        t0 -= cz;
        t1 -= cz;
        t2 -= cz;
        t3 -= cz;
        t4 -= cz;
        t5 -= cz;
        t6 -= cz;
        t7 -= cz;

        sprintf(msg, "z stats %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
                range, step, t0, t1, t2, t3, t4, t5, t6, t7);
        log()->get(LogLevel::Debug2) << msg << std::endl;

        int id0, id1, id2, id3, id4, id5, id6, id7, id8;
        id0 = id1 = id2 = id3 = id4 = id5 = id6 = id7 = id8 = 0;

        for (point_count_t i = 0; i < view->size(); ++i)
        {
            double xd = view->getFieldAs<double>(Dimension::Id::X, i) - cx;
            double yd = view->getFieldAs<double>(Dimension::Id::Y, i) - cy;
            double zd = view->getFieldAs<double>(Dimension::Id::Z, i) - cz;
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

        sprintf(msg, "ids: %d %d %d %d %d %d %d %d %d", id0, id1,
                id2, id3, id4, id5, id6, id7, id8);
        log()->get(LogLevel::Debug2) << msg << std::endl;

        m_prcFile->addPoints(id0, const_cast<const double**>(p0), c0, 1.0);
        m_prcFile->addPoints(id1, const_cast<const double**>(p1), c1, 1.0);
        m_prcFile->addPoints(id2, const_cast<const double**>(p2), c2, 1.0);
        m_prcFile->addPoints(id3, const_cast<const double**>(p3), c3, 1.0);
        m_prcFile->addPoints(id4, const_cast<const double**>(p4), c4, 1.0);
        m_prcFile->addPoints(id5, const_cast<const double**>(p5), c5, 1.0);
        m_prcFile->addPoints(id6, const_cast<const double**>(p6), c6, 1.0);
        m_prcFile->addPoints(id7, const_cast<const double**>(p7), c7, 1.0);
        m_prcFile->addPoints(id8, const_cast<const double**>(p8), c8, 1.0);

        for (point_count_t i = 0; i < view->size(); ++i)
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
        log()->get(LogLevel::Debug4) << "No color scheme provided." << std::endl;

        bool bHaveColor(false);
        if (view->hasDim(Dimension::Id::Red) &&
            view->hasDim(Dimension::Id::Green) &&
            view->hasDim(Dimension::Id::Blue))
            bHaveColor = true;

        if (bHaveColor)
        {
            log()->get(LogLevel::Debug4) << "Using RGB." << std::endl;

            uint16_t histogram[INT16_MAX] = {0};

            double xd(0.0);
            double yd(0.0);
            double zd(0.0);

            for (point_count_t i = 0; i < view->size(); ++i)
            {
                uint16_t r = view->getFieldAs<uint16_t>(
                    Dimension::Id::Red, i);
                uint16_t g = view->getFieldAs<uint16_t>(
                    Dimension::Id::Green, i);
                uint16_t b = view->getFieldAs<uint16_t>(
                    Dimension::Id::Blue, i);
                uint16_t color = RGB(r, g, b);
                histogram[color]++;
            }

            byte colMap[256][3];
            ColorQuantizer *colorQuantizer = new ColorQuantizer();
            // is there any chance that this won't return 256 cubes? should we check?
            word ncubes = colorQuantizer->medianCut(histogram, colMap, 256);

            std::vector<std::vector<int> > indices;
            indices.resize(256);

            for (point_count_t i = 0; i < view->size(); ++i)
            {
                uint16_t r = view->getFieldAs<uint16_t>(
                    Dimension::Id::Red, i);
                uint16_t g = view->getFieldAs<uint16_t>(
                    Dimension::Id::Green, i);
                uint16_t b = view->getFieldAs<uint16_t>(
                    Dimension::Id::Blue, i);
                uint16_t color = RGB(r, g, b);
                uint16_t colorIndex = histogram[color];
                indices[colorIndex].push_back(i);
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

                    xd = view->getFieldAs<double>(Dimension::Id::X, idx) - cx;
                    yd = view->getFieldAs<double>(Dimension::Id::Y, idx) - cy;
                    zd = view->getFieldAs<double>(Dimension::Id::Z, idx) - cz;

                    points[point][0] = xd;
                    points[point][1] = yd;
                    points[point][2] = zd;
                    numPoints++;
                }

                double r = static_cast<double>((int)(colMap[level][0])/255.0);
                double g = static_cast<double>((int)(colMap[level][1])/255.0);
                double b = static_cast<double>((int)(colMap[level][2])/255.0);

                m_prcFile->addPoints(num_points,
                                     const_cast<const double**>(points),
                                     RGBAColour(r, g, b, 1.0), 5.0);

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
            log()->get(LogLevel::Debug4) << "Using solid color." << std::endl;

            double **points;
            points = (double**) malloc(view->size()*sizeof(double*));
            for (point_count_t i = 0; i < view->size(); ++i)
            {
                points[i] = (double*) malloc(3*sizeof(double));
            }

            double xd(0.0);
            double yd(0.0);
            double zd(0.0);

            for (point_count_t i = 0; i < view->size(); ++i)
            {
                xd = view->getFieldAs<double>(Dimension::Id::X, i) - cx;
                yd = view->getFieldAs<double>(Dimension::Id::Y, i) - cy;
                zd = view->getFieldAs<double>(Dimension::Id::Z, i) - cz;

                if (i % 10000 == 0)
                {
                    char msg[1000];
                    sprintf(msg, "small point %f %f %f", xd, yd, zd);
                    log()->get(LogLevel::Debug2) << msg << std::endl;
                }
                points[i][0] = xd;
                points[i][1] = yd;
                points[i][2] = zd;

                numPoints++;
            }

            m_prcFile->addPoints(numPoints, const_cast<const double**>(points),
                                 RGBAColour(1.0,1.0,0.0,1.0),1.0);

            for (point_count_t i = 0; i < view->size(); ++i)
            {
                free(points[i]);
            }
            free(points);
        }
    }
}

std::istream& operator>>(std::istream& in, PrcWriter::OutputFormat& fmt)
{
    std::string s;
    in >> s;

    s = Utils::tolower(s);
    if (s == "pdf")
        fmt = PrcWriter::OutputFormat::Pdf;
    else if (s == "prc")
        fmt = PrcWriter::OutputFormat::Prc;
    else
        in.setstate(std::ios::failbit);
    return in;
}

std::ostream& operator<<(std::ostream& out, const PrcWriter::OutputFormat& fmt)
{
    switch (fmt)
    {
    case PrcWriter::OutputFormat::Pdf:
        out << "Pdf";
    case PrcWriter::OutputFormat::Prc:
        out << "Prc";
    }
    return out;
}

std::istream& operator>>(std::istream& in, PrcWriter::ColorScheme& scheme)
{
    std::string s;
    in >> s;

    s = Utils::tolower(s);
    if (s == "solid")
        scheme = PrcWriter::ColorScheme::Solid;
    else if (s == "oranges")
        scheme = PrcWriter::ColorScheme::Oranges;
    else if (s == "bluegreen")
        scheme = PrcWriter::ColorScheme::BlueGreen;
    else
        in.setstate(std::ios::failbit);
    return in;
}

std::ostream& operator<<(std::ostream& out,
    const PrcWriter::ColorScheme& scheme)
{
    switch (scheme)
    {
    case PrcWriter::ColorScheme::Solid:
        out << "Solid";
    case PrcWriter::ColorScheme::Oranges:
        out << "Oranges";
    case PrcWriter::ColorScheme::BlueGreen:
        out << "BlueGreen";
    }
    return out;
}

std::istream& operator>>(std::istream& in, PrcWriter::ContrastStretch& cs)
{
    std::string s;
    in >> s;

    s = Utils::tolower(s);
    if (s == "linear")
        cs = PrcWriter::ContrastStretch::Linear;
    else if (s == "sqrt")
        cs = PrcWriter::ContrastStretch::Sqrt;
    else
        in.setstate(std::ios::failbit);
    return in;
}

std::ostream& operator<<(std::ostream& out,
    const PrcWriter::ContrastStretch& cs)
{
    switch (cs)
    {
    case PrcWriter::ContrastStretch::Linear:
        out << "Linear";
    case PrcWriter::ContrastStretch::Sqrt:
        out << "Sqrt";
    }
    return out;
}

} // namespace pdal
