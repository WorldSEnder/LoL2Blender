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
#ifndef RIOT__SKLDATA_HPP
#define RIOT__SKLDATA_HPP

#include <vector>

#include <maya/MDagPathArray.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MQuaternion.h>
#include <maya/MVector.h>

namespace riot {

struct SklBone
{
    static const int kNameLen = 0x20;
    static const int kSizeWithoutMatrix = 0x28;
    static const int kSizeInFile = 0x58;

    SklBone()
    {
        scale = 0.1f;
        memset(name, 0, kNameLen);
    }
    
    char name[kNameLen];
    int parent;
    float scale; // 0.1f
    float transform[4][4];
};

struct SklData
{
    // the HLSL shader says 64 bones,
    // but in fact the 4 next registers are not used, it won't make the pipeline bug
    static const int kMaxIndices = 0x44;

    void switchHand()
    {
        int bones_size = static_cast<int>(bones.size());
        for (int i = 0; i < bones_size; i++)
        {
            SklBone& bone = bones.at(i);
            MMatrix mat(bone.transform);
            MTransformationMatrix transMat(mat);
            MQuaternion rot = transMat.rotation();
            transMat.setRotationQuaternion(rot.x, -rot.y, -rot.z, rot.w);
            mat = transMat.asMatrix();
            /*
            in maya : 
            T  =    |  1    0    0    0 |
                    |  0    1    0    0 |
                    |  0    0    1    0 |
                    |  tx   ty   tz   1 |
            http://download.autodesk.com/us/maya/2011help/API/class_m_transformation_matrix.html
            */
            mat[3][0] = -mat[3][0];
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    bone.transform[j][k] = static_cast<float>(mat[j][k]);
        }
    }
    
    int version;
    int num_bones;
    std::vector<SklBone> bones;
    int num_indices;
    MIntArray skn_indices;
    MDagPathArray joints;
};

} // namespace riot

#endif

