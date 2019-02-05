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

#include "ColorQuantizer.hpp"

#include <algorithm>
#include <iostream>

namespace pdal
{

ColorQuantizer::ColorQuantizer()
{
}

word ColorQuantizer::medianCut(word hist[], byte colMap[][3], int maxCubes)
{
    byte    lr, lg, lb;
    word    i, median, color;
    long    count;
    int     k, level, ncubes, splitpos;
    void    *base;
    size_t  num, width;
    cube_t  cube, cubeA, cubeB;

    //Create initial cube
    ncubes = 0;
    cube.count = 0;
    for (i=0, color=0; i<=HSIZE-1; i++)
    {
        if (hist[i] != 0)
        {
            histPtr[color++] = i;
            cube.count = cube.count + hist[i];
        }
    }

    cube.lower = 0;
    cube.upper = color-1;
    cube.level = 0;
    shrink(&cube);
    cubeList[ncubes++] = cube;

    //main loop
    while (ncubes < maxCubes)
    {
        level = 255;
        splitpos = -1;
        for (k = 0; k <= ncubes-1; k++)
        {
            if ((cubeList[k].lower != cubeList[k].upper) &&
                    cubeList[k].level < level)
            {
                level = cubeList[k].level;
                splitpos = k;
            }
        }
        if (splitpos == -1)
        {
            break;
        }
        cube = cubeList[splitpos];
        lr = cube.rmax - cube.rmin;
        lg = cube.gmax - cube.gmin;
        lb = cube.bmax - cube.bmin;
        if (lr >= lg && lr >= lb) longdim = 0;
        if (lg >= lr && lg >= lb) longdim = 1;
        if (lb >= lr && lb >= lg) longdim = 2;

        base = (void *)&histPtr[cube.lower];
        num = (size_t)(cube.upper - cube.lower + 1);
        width = (size_t)sizeof(histPtr[0]);
        qsort(base,num,width,compare);

        //Find median
        count = 0;
        for (i=cube.lower; i<=cube.upper-1; i++)
        {
            if (count >= cube.count/2) break;
            color = histPtr[i];
            count = count + hist[color];
        }
        median = i;

        //Split cube at the median.
        cubeA = cube;
        cubeA.upper = median - 1;
        cubeA.count = count;
        cubeA.level = cube.level + 1;
        shrink(&cubeA);
        cubeList[splitpos] = cubeA;

        cubeB = cube;
        cubeB.lower = median;
        cubeB.count = cube.count - count;
        cubeB.level = cube.level + 1;
        shrink(&cubeB);
        cubeList[ncubes++] = cubeB;

        if ((ncubes % 10) == 0)
        {
            std::cerr << ".";
        }
    }

    invMap(hist, colMap, ncubes);

    return ((word)ncubes);
}

void ColorQuantizer::invMap(word *hist, byte colMap[][3], word ncubes)
{
    byte      r,g,b;
    word      i,j,k,index,color;
    float     rsum,gsum,bsum;
    float     dr,dg,db,d,dmin;
    cube_t    cube;

    for (k=0; k<=ncubes-1; k++)
    {
        cube = cubeList[k];
        rsum = gsum = bsum = (float) 0.0;
        for (i=cube.lower; i<=cube.upper; i++)
        {
            color = histPtr[i];
            r = RED(color);
            rsum += (float)r*(float)hist[color];
            g = GREEN(color);
            gsum += (float)g*(float)hist[color];
            b = BLUE(color);
            bsum += (float)b*(float)hist[color];
        }

        //Update the color map
        colMap[k][0] = (byte)(rsum/(float)cube.count);
        colMap[k][1] = (byte)(gsum/(float)cube.count);
        colMap[k][2] = (byte)(bsum/(float)cube.count);
    }

    for (k=0; k<=ncubes-1; k++)
    {
        cube = cubeList[k];
        for (i=cube.lower; i<=cube.upper; i++)
        {
            color = histPtr[i];
            hist[color] = k;
        }

        if ((k % 10) == 0)
            std::cerr << ".";
    }
    return;
}

void ColorQuantizer::shrink(cube_t *cube)
{
    byte    r, g, b;
    word    i,color;

    cube->rmin = 255;
    cube->rmax = 0;
    cube->gmin = 255;
    cube->gmax = 0;
    cube->bmin = 255;
    cube->bmax = 0;
    for (i = cube->lower; i <= cube->upper; i++)
    {
        color = histPtr[i];
        r = RED(color);
        if (r > cube->rmax) cube->rmax = r;
        if (r < cube->rmin) cube->rmin = r;
        g = GREEN(color);
        if (g > cube->gmax) cube->gmax = g;
        if (g < cube->gmin) cube->gmin = g;
        b = BLUE(color);
        if (b > cube->bmax) cube->bmax = b;
        if (b < cube->bmin) cube->bmin = b;
    }
}

int ColorQuantizer::compare(const void *a1, const void *a2)
{
    word color1, color2;
    byte c_1, c_2;

    color1 = *(reinterpret_cast<const word *>(a1));
    color2 = *(reinterpret_cast<const word *>(a2));
    switch (longdim)
    {
        case 0:
            c_1 = RED(color1), c_2 = RED(color2);
            break;
        case 1:
            c_1 = GREEN(color1), c_2 = GREEN(color2);
            break;
        case 2:
            c_1 = BLUE(color1), c_2 = BLUE(color2);
            break;
    }
    return ((int)(c_1 - c_2));
}

} //namespace pdal
