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
#ifndef RIOT__SKNREADER_H
#define RIOT__SKNREADER_H

#include <vector>

#include <maya/MStatus.h>
#include <maya/MIOStream.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MVectorArray.h>

#include <SklData.hpp>
#include <SknData.hpp>

#ifndef nullptr
#define nullptr 0
#endif

namespace riot {

class SknReader
{
public:
    MStatus read(istream& file);
    MStatus loadData(MString& name,
                        bool use_normals = true,
                        SklData* skl_data = nullptr);

    SknData data_;

private:
    MIntArray poly_counts;
    MIntArray poly_connects;
    MFloatArray u_array;
    MFloatArray v_array;
    MVectorArray normals;
    MIntArray normals_indices;
};

} // namespace riot

#endif

