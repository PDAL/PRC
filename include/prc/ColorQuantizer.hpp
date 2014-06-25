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

#ifndef _COLOR_QUANTIZER_H
#define _COLOR_QUANTIZER_H

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#define HSIZE 32768
#define MAXCOLORS 256
#define RGB(r,g,b) (word)(((b)&~7)<<7)|(((g)&~7)<<2)|((r)>>3)
#define RED(x)     (byte)(((x)&31)<<3)
#define GREEN(x)   (byte)((((x)>>5)&255)<< 3)
#define BLUE(x)    (byte)((((x)>>10)&255)<< 3)

namespace pdal
{
namespace drivers
{
namespace prc
{

typedef unsigned char byte;
typedef uint16_t word;
typedef struct
{
    word lower;
    word upper;
    long     count;
    int      level;

    byte     rmin, rmax;
    byte     gmin, gmax;
    byte     bmin, bmax;
} cube_t;

static int longdim;
static word histPtr[HSIZE];

/**
 * MedianCut color quantization adapted from
 * http://collaboration.cmc.ec.gc.ca/science/rpn/biblio/ddj/Website/articles/DDJ/1994/9409/9409e/9409e.htm
 */
class ColorQuantizer
{
private:
    cube_t cubeList[MAXCOLORS];

    void shrink(cube_t *cube);
    void invMap(word *hist, byte colMap[][3], word ncubes);
    static int compare(const void *a1, const void *a2);

public:
    ColorQuantizer();
    word medianCut(word hist[], byte ColMap[][3], int maxcubes);
};

}  // prc
}  // drivers
}  // pdal

#endif //_COLOR_QUANTIZER_H

