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
#ifndef RIOT__SCODATA_HPP
#define RIOT__SCODATA_HPP

#include <vector>

#include <maya/MString.h>

namespace riot {

const int kMaxBufLen = 0x100;

struct ScoMaterial
{
    static const int kMaxNameLen = 0x50; // it seems in fact the limit is 32k oO !!
    MString name;
};

struct ScoVtx
{
    float x;
    float y;
    float z;
};

struct ScoData
{
    static const int kMaxNameLen = 0x80 - 1;

    ScoData() { use_pivot = false; }

    void switchHand()
    {
        int vertices_size = static_cast<int>(vertices.size());
        for (int i = 0; i < vertices_size; i++)
            vertices.at(i).x = -vertices.at(i).x;
        
        tx = -tx;
        px = -px;
    }

    MString name;
    float tx;
    float ty;
    float tz;
    bool use_pivot;
    float px;
    float py;
    float pz;
    int num_vtxs;
    int num_indices;
    std::vector<ScoMaterial> materials;
    std::vector<ScoVtx> vertices;
    std::vector<int> indices;
    std::vector<int> shader_per_triangle;
    std::vector<double> u_vec;
    std::vector<double> v_vec;
};

} // namespace riot

#endif

