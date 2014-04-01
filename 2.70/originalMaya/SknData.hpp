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
#ifndef RIOT__SKNDATA_HPP
#define RIOT__SKNDATA_HPP

#include <vector>
#include <map>

#include <maya/MMatrix.h>
#include <maya/MIntArray.h>

namespace riot {

struct SknMaterial
{
    static const int kSizeInFile = 0x50;
    static const int kNameLen = 0x40;

    SknMaterial() { memset(name, 0, kNameLen); }

    char name[kNameLen];
    int startVertex;
    int num_vertices;
    int startIndex;
    int num_indices;
};

struct SknVtx
{
    static const int kSizeInFile = 0x34;

    SknVtx()
    {
        for (int i = 0; i < 4; i++)
        {
            skn_indices[i] = 0;
            weights[i] = 0;
        }
    }

    float x;
    float y;
    float z;
    char skn_indices[4];
    float weights[4];
    float normal[3];
    float U;
    float V;
    int skl_indices[4];
    int uv_index;
    int dupe_data_index;
};

class SknData
{
public:
    void switchHand()
    {
        int vertices_size = static_cast<int>(vertices.size());
        for (int i = 0; i < vertices_size; i++)
        {
            SknVtx& vtx = vertices.at(i);
            vtx.x = -vtx.x;
            vtx.normal[1] = -vtx.normal[1];
            vtx.normal[2] = -vtx.normal[2];
        }
    }

    short version;
    int num_vtxs;
    int num_indices;
    int num_final_vtxs;
    std::vector<USHORT> indices;
    std::vector<SknMaterial> materials;
    std::vector<SknVtx> vertices;
    int endTab[3];
};

} // namespace riot

#endif

