/*
    Copyright 2011 Even Entem (alias ThiSpawn).

    This file is part of Riot File Translator Plug-in for AutoDesk Maya.

    Autodesk Maya's lib is under :
    Copyright 1995, 2006, 2008 Autodesk, Inc. All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful, 
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef RIOT__SCBDATA_HPP
#define RIOT__SCBDATA_HPP

#include <vector>

#include <maya/MString.h>
#include <maya/MColor.h>

namespace riot {

struct ScbMaterial
{
    static const int kNameLen = 0x40;

    ScbMaterial() { memset(name, 0, kNameLen); }

    char name[kNameLen];
};

struct ScbVtx
{
    static const int kSizeInFile = 0xC;

    float x;
    float y;
    float z;
};

struct ScbData
{
    static const int kNameLen = 0x80;

    ScbData()
    {
        memset(name, 0, kNameLen);
        is_colored = true;
    }

    void switchHand()
    {
        int vertices_size = static_cast<int>(vertices.size());
        for (int i = 0; i < vertices_size; i++)
            vertices.at(i).x = -vertices.at(i).x;

        bbx = -bbx;
        bbdx = -bbdx;
    }

    int version;
    char name[kNameLen];
    float bbx;
    float bby;
    float bbz;
    float bbdx;
    float bbdy;
    float bbdz;
    int num_vtxs;
    int num_indices;
    std::vector<ScbMaterial> materials;
    std::vector<ScbVtx> vertices;
    std::vector<int> indices;
    std::vector<int> shader_per_triangle;
    std::vector<double> u_vec;
    std::vector<double> v_vec;
    bool is_colored;
    std::vector<MColor> colors; // RGBA
};

} // namespace riot

#endif

