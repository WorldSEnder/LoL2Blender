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
#ifndef RIOT__ANMDATA_HPP
#define RIOT__ANMDATA_HPP

#include <vector>

#include <maya/MDagPathArray.h>
#include <maya/MTransformationMatrix.h>

namespace riot {

struct Vec3
{
    float x;
    float y;
    float z;
};

struct Quat
{
    float q[4];
};

struct AnmPos
{
    static const int kSizeInFile = 0x1C;

    float rot[4];
    float x;
    float y;
    float z;
};

struct AnmBone
{
    static const int kHeaderSize = 0x24;
    static const int kNameLen = 0x20;

    AnmBone()
        : name_hash(0)
    {
        memset(name, 0, kNameLen);
    }

    char name[kNameLen];
    int flag; // 2 if it is a root, 0 else.
    int name_hash;
    std::vector<AnmPos> poses;
};

struct AnmData
{
    void switchHand()
    {
        int bones_size = static_cast<int>(bones.size());
        for (int i = 0; i < bones_size; i++)
        {
            AnmBone& bone = bones.at(i);
            for (int j = 0; j < num_frames; j++)
            {
                AnmPos& pos = bone.poses.at(j);
                pos.rot[1] = -pos.rot[1];
                pos.rot[2] = -pos.rot[2];
                pos.x = -pos.x;
            }
        }
    }

    int version; // 3 | 4
    int num_bones;
    int num_frames;
    float fps;
    std::vector<AnmBone> bones;
    MDagPathArray joints;
};

} // namespace riot

#endif

