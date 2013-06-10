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
  std::cout << "writer\n";
  std::cout << options.getOption("prc_filename").getValue<std::string>() << std::endl;

  return;
}


Writer::~Writer()
{
  return;
}


void Writer::initialize()
{
  std::cout << "init\n";

  pdal::Writer::initialize();

  std::cout << "succes\n";

  std::string output_format = getOptions().getValueOrDefault<std::string>("output_format", "pdf");

  if(boost::iequals(output_format, "pdf"))
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

  if(boost::iequals(color_scale, "none"))
    m_colorScale = COLOR_SCALE_NONE;
  else if (boost::iequals(color_scale, "auto"))
    m_colorScale = COLOR_SCALE_AUTO;
  else
  {
    std::ostringstream oss;
    oss << "Unrecognized color scale " << color_scale;
    throw prc_driver_error("Unrecognized color scale");
  }
  std::cout << color_scale << " scale" << std::endl;

  return;
}



Options Writer::getDefaultOptions()
{
  Options options;

  Option prc_filename("prc_filename", "", "Filename to write PRC file to");
  Option pdf_filename("pdf_filename", "", "Filename to write PDF file to");
  Option output_format("output_format", "", "PRC or PDF");
  Option color_scale("color_scale", "", "None or auto");

  options.add(prc_filename);
  options.add(pdf_filename);
  options.add(output_format);
  options.add(color_scale);

  return options;
}


void Writer::writeBegin(boost::uint64_t /*targetNumPointsToWrite*/)
{
  PRCoptions grpopt;
  grpopt.no_break = true;
  grpopt.do_break = false;
  grpopt.tess = true;

  m_prcFile.begingroup("points",&grpopt);

  std::cout << "begin\n";

  return;
}


void Writer::writeEnd(boost::uint64_t /*actualNumPointsWritten*/)
{
  std::cout << "end\n";

  m_prcFile.endgroup();
  m_prcFile.finish();

  std::cout << "format is " << m_outputFormat << std::endl;

  if(m_outputFormat == OUTPUT_FORMAT_PDF)
  {
    std::cout << "detected PDF\n";

    const double width = 256.0f;
    const double height = 256.0f;
    const double depth = std::sqrt(width*height);

    std::cout << width << " " << height << " " << depth << std::endl;

    const HPDF_Rect rect = { 0, 0, width, height };
    HPDF_Doc pdf;
    HPDF_Page page;
    HPDF_Annotation annot;
    HPDF_U3D u3d;
    HPDF_Dict view;

    pdf = HPDF_New( NULL, NULL );
    if (!pdf)
    {
      printf("error: cannot create PdfDoc object\n");
      return;
    }
    std::cout << HPDF_GetError(pdf) << std::endl;
    pdf->pdf_version = HPDF_VER_17;

    std::cout << "pdf version " << pdf->pdf_version << std::endl;

    page = HPDF_AddPage( pdf );
    HPDF_Page_SetWidth( page, width );
    HPDF_Page_SetHeight( page, height );

    std::string prcFilename = getOptions().getValueOrThrow<std::string>("prc_filename");
    std::cout << prcFilename;
    u3d = HPDF_LoadU3DFromFile( pdf, prcFilename.c_str() );
    if (!u3d)
    {
      printf("error: cannot load U3D object\n");
      return;
    }
    std::cout << HPDF_GetError(pdf) << std::endl;
    std::cout << " loaded\n";

    view = HPDF_Create3DView( u3d->mmgr, "DefaultView" );
    if (!view)
    {
      printf("error: cannot create view\n");
      return;
    }
    std::cout << "create view: " << HPDF_GetError(pdf) << std::endl;

    std::cout << "default view\n";

    HPDF_REAL coox = getCOOx();
    HPDF_REAL cooy = getCOOy();
    HPDF_REAL cooz = getCOOz();
    std::cout << "set cooz: " << HPDF_GetError(pdf) << std::endl;

    coox = cooy = 0.0;

    printf("%f %f %f\n", coox, cooy, cooz);

    HPDF_3DView_SetCamera( view, coox, cooy, cooz, 0, 0, 1, 4*cooz, 0 );
    std::cout << "set camera: " << HPDF_GetError(pdf) << std::endl;
    HPDF_3DView_SetPerspectiveProjection( view, 30.0 );
    std::cout << "set perspective: " << HPDF_GetError(pdf) << std::endl;
    HPDF_3DView_SetBackgroundColor( view, 0, 0, 0 );
    std::cout << "set background: " << HPDF_GetError(pdf) << std::endl;
    HPDF_3DView_SetLighting( view, "Headlamp" );
    std::cout << "set lighting: " << HPDF_GetError(pdf) << std::endl;

    std::cout << "view created\n";

    HPDF_U3D_Add3DView( u3d, view );
    std::cout << "add view: " << HPDF_GetError(pdf) << std::endl;
    HPDF_U3D_SetDefault3DView( u3d, "DefaultView" );
    std::cout << "set default view: " << HPDF_GetError(pdf) << std::endl;

    annot = HPDF_Page_Create3DAnnot( page, rect, u3d );
    if (!annot)
    {
      printf("error: cannot create annotation\n");
      return;
    }
    std::cout << "create annotation: " << HPDF_GetError(pdf) << std::endl;

    //HPDF_Dict action = (HPDF_Dict) HPDF_Dict_GetItem( annot, "3DA", HPDF_OCLASS_DICT );
    //HPDF_Dict_AddBoolean( action, "TB", HPDF_TRUE );

    std::cout << "annotated\n";

    std::string pdfFilename = getOptions().getValueOrThrow<std::string>("pdf_filename");
    std::cout << pdfFilename;
    HPDF_STATUS success = HPDF_SaveToFile( pdf, pdfFilename.c_str() );
    std::cout << " saved\n";
    std::cout << success << std::endl;
    std::cout << HPDF_GetErrorDetail(pdf) << std::endl;

    HPDF_Free( pdf );
  }

  return;
}

boost::uint32_t Writer::writeBuffer(const PointBuffer& data)
{
  std::cout << "write\n";

  boost::uint32_t numPoints = 0;

  const pdal::Bounds<double>& bounds = data.getSpatialBounds();

  double cx = (bounds.getMaximum(0)-bounds.getMinimum(0))/2+bounds.getMinimum(0);
  double cy = (bounds.getMaximum(1)-bounds.getMinimum(1))/2+bounds.getMinimum(1);
  double cz = (bounds.getMaximum(2)-bounds.getMinimum(2))/2+bounds.getMinimum(2);

  setCOOx(static_cast<HPDF_REAL>((bounds.getMaximum(0)-bounds.getMinimum(0))/2+bounds.getMinimum(0)));
  setCOOy(static_cast<HPDF_REAL>((bounds.getMaximum(1)-bounds.getMinimum(1))/2+bounds.getMinimum(1)));
  setCOOz(static_cast<HPDF_REAL>((bounds.getMaximum(2)-bounds.getMinimum(2))/2+bounds.getMinimum(2)));

  pdal::Schema const& schema = data.getSchema();

  pdal::Dimension const& dimX = schema.getDimension("X");
  pdal::Dimension const& dimY = schema.getDimension("Y");
  pdal::Dimension const& dimZ = schema.getDimension("Z");

  if (m_colorScale == COLOR_SCALE_AUTO)
  {
    std::cout << "auto scaling\n";

    double **points;
    points = (double**) malloc(sizeof(double*));
    {
      points[0] = (double*) malloc(3*sizeof(double));
    }

    double xd(0.0);
    double yd(0.0);
    double zd(0.0);

    for(boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
    {
      boost::int32_t x = data.getField<boost::int32_t>(dimX, i);
      boost::int32_t y = data.getField<boost::int32_t>(dimY, i);
      boost::int32_t z = data.getField<boost::int32_t>(dimZ, i);

      xd = dimX.applyScaling<boost::int32_t>(x) - cx;
      yd = dimY.applyScaling<boost::int32_t>(y) - cy;
      zd = dimZ.applyScaling<boost::int32_t>(z) - cz;

      double r = (dimZ.applyScaling<boost::int32_t>(z) - bounds.getMinimum(2)) / (bounds.getMaximum(2) - bounds.getMinimum(2));
      double g, b;
      g = b = 0.0f;
      if(i % 1000) printf("%f %f %f %f %f %f\n", xd, yd, zd, r, g, b);

      points[0][0] = xd;
      points[0][1] = yd;
      points[0][2] = zd;

      m_prcFile.addPoints(1, const_cast<const double**>(points), RGBAColour(r,0.0,0.0,1.0), 5.0);

      numPoints++;
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
      std::cout << "with rgb\n";

      double **points;
      points = (double**) malloc(sizeof(double*));
      {
        points[0] = (double*) malloc(3*sizeof(double));
      }

      double xd(0.0);
      double yd(0.0);
      double zd(0.0);

      for(boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
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

        if(i % 1000) printf("%f %f %f %f %f %f\n", xd, yd, zd, r, g, b);

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
      std::cout << "no rgb\n";

      double **points;
      points = (double**) malloc(data.getNumPoints()*sizeof(double*));
      for (boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
      {
        points[i] = (double*) malloc(3*sizeof(double));
      }

      double xd(0.0);
      double yd(0.0);
      double zd(0.0);

      for(boost::uint32_t i = 0; i < data.getNumPoints(); ++i)
      {
        boost::int32_t x = data.getField<boost::int32_t>(dimX, i);
        boost::int32_t y = data.getField<boost::int32_t>(dimY, i);
        boost::int32_t z = data.getField<boost::int32_t>(dimZ, i);

        xd = dimX.applyScaling<boost::int32_t>(x) - cx;
        yd = dimY.applyScaling<boost::int32_t>(y) - cy;
        zd = dimZ.applyScaling<boost::int32_t>(z) - cz;

  //      if(i % 10000) printf("%f %f %f\n", xd, yd, zd);

        points[i][0] = xd;
        points[i][1] = yd;
        points[i][2] = zd;

        numPoints++;
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
