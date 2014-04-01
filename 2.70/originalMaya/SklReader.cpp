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

#include <SklReader.h>

#include <maya/MGlobal.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnDagNode.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MMatrix.h>
#include <maya/MVector.h>
#include <maya/MDagPath.h>
#include <maya/MQuaternion.h>

#include <maya_misc.h>

namespace riot {

struct RawHeader
{
    int size;
    int magic;
    int uk;
    WORD uk2;
    WORD nbSklBones;
    int num_bones_foranim;
    int header_size; // 0x40
    int size_after_array1;
    int size_after_array2;
    int size_after_array3;
    int size_after_array3_; // duped ..
    int size_after_array4;
};

struct RawSklBone
{
    WORD uk;
    short id;
    short parent_id;
    WORD uk2;
    int namehash;
    float unused;
    float tx; 
    float ty; 
    float tz;
    float unused1;
    float unused2;
    float unused3;
    float q1;
    float q2;
    float q3;
    float q4;
    float ctx;
    float cty;
    float ctz;
};

MStatus SklReader::readBinary(istream& file)
{
    // get length
    file.seekg(0, ios::end);
    int length = file.tellg();
    file.seekg(0, ios::beg);
    // copy in mem
    char* pcopy = reinterpret_cast<char*>(malloc((length + 3) & 0xFFFFFFFC)); // align 4
    file.read(pcopy, length);

    data_.version = 3;

    struct RawHeader* phead = reinterpret_cast<struct RawHeader*>(pcopy);

    int num_bones = phead->nbSklBones;
    data_.num_bones = num_bones;

    struct RawSklBone* raw_bone = reinterpret_cast<struct RawSklBone*>(pcopy + phead->header_size);
    char* pname = pcopy + phead->size_after_array4;
    
    data_.bones.resize(num_bones);
    
    // get bones
    for (int i = 0; i < num_bones; i++)
    {
        SklBone bone;
        
        MVector translation = MVector(raw_bone->tx, raw_bone->ty, raw_bone->tz);
        MTransformationMatrix transform;
        transform.setTranslation(translation, MSpace::kWorld);
        transform.setRotationQuaternion(raw_bone->q1, raw_bone->q2, raw_bone->q3, raw_bone->q4, MSpace::kWorld);
        MMatrix mat = transform.asMatrix();
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                bone.transform[j][k] = static_cast<float>(mat[j][k]);
        
        char* c = bone.name;
        int count = 0;
        char* pname0 = pname;
        while (*pname0 != 0 && count < SklBone::kNameLen - 1)
        {
            *c++ = *pname0++;
            count++;
        }
        count = (count + 4) & 0xFFFFFFFC;

        bone.parent = raw_bone->parent_id;
        data_.bones[raw_bone->id] = bone;
        raw_bone = reinterpret_cast<struct RawSklBone*>(reinterpret_cast<char*>(raw_bone) + 0x64);
        pname += count;
    }

    int num_indices = phead->num_bones_foranim;
    data_.num_indices = num_indices;
    
    WORD* anim_indices = reinterpret_cast<WORD*>(pcopy + phead->size_after_array2);
    
    for (int i = 0; i < num_indices; i++)
    {
        data_.skn_indices.append(*anim_indices);
        anim_indices++;
    }

    free(pcopy);

    return MS::kSuccess;
}

MStatus SklReader::read(istream& file)
{
    // get length
    int minlen = 20;
    file.seekg(0, ios::end);
    int length = file.tellg();
    file.seekg(0, ios::beg);

    // check minimum length
    if (length < minlen)
        FAILURE("SklReader: the file is empty!");

    // check magic
    char magic[8];
    file.read(magic, 8);
    if (!strncmp(magic, "r3d2sklt", 8))
    {
        // get version
        int version;
        file.read(reinterpret_cast<char*>(&version), 4);
        if (version != 2 && version != 1)
            FAILURE("SklReader: skl type not supported, \n please report that to ThiSpawn");
        data_.version = version;

        // get designer ID
        int designerId;
        file.read(reinterpret_cast<char*>(&designerId), 4);

        // get num_bones
        int num_bones;
        file.read(reinterpret_cast<char*>(&num_bones), 4);

        // check minimum length
        minlen += num_bones * SklBone::kSizeInFile;
        if (version == 2)
            minlen += 4;
        if (length < minlen)
            FAILURE("SklReader: unexpected end of file");
        data_.num_bones = num_bones;

        // get bones
        for (int i = 0; i < num_bones; i++)
        {
            SklBone bone;
            file.read(reinterpret_cast<char*>(&bone), SklBone::kSizeWithoutMatrix);
            for (int j = 0; j < 3; j++)
                for (int k = 0; k < 4; k++)
                    file.read(reinterpret_cast<char*>(&(bone.transform[k][j])), 4);
            bone.transform[0][3] = 0.0f;
            bone.transform[1][3] = 0.0f;
            bone.transform[2][3] = 0.0f;
            bone.transform[3][3] = 1.0f;
            data_.bones.push_back(bone);
        }

        // get end tab
        if (version == 2)
        {
            int num_indices;
            file.read(reinterpret_cast<char*>(&num_indices), 4);
            minlen += num_indices * 4;
            if (length < minlen)
                FAILURE("SklReader: unexpected end of file");
            // cassiopeia exceed the vertex shader limitations, so i remove this test
            //if (num_indices > data_.max_indices)
                //FAILURE("SklReader: too much skn indices");
            data_.num_indices = num_indices;
            for (int i = 0; i < num_indices; i++)
            {
                int sknID;
                file.read(reinterpret_cast<char*>(&sknID), 4);
                data_.skn_indices.append(sknID);
            }
        }
        else if (num_bones > data_.kMaxIndices)
        {
            MGlobal::displayWarning("SklReader: skl is of type 1 and should be of type 2 (too much bones)");
        }
        else
        {
            data_.num_indices = num_bones;
            for (int i = 0; i < num_bones; i++)
            {
                data_.skn_indices.append(i);
            }
        }
    }
    else if (*reinterpret_cast<int*>(magic + 4) == 0x22FD4FC3)
    {
        data_.version = 3;
        MGlobal::displayInfo("SklReader: skl is of type raw, this support is in beta test, report any problems.");
        readBinary(file);
    }
    else
        FAILURE("SklReader: magic is wrong!");
    
    data_.switchHand();

    return MS::kSuccess;
}

MStatus SklReader::loadData()
{
    // the bones don't need to be in hierarchical order
    // prevent update for later type versions.

    MFnIkJoint fn_joint;
    
    MStatus status;
    MDagPath dag_path;

    data_.joints.clear();

    // create bones
    for (int i = 0; i < data_.num_bones; i++)
    {
        SklBone bone = data_.bones.at(i);
        MMatrix mat(bone.transform);
        MTransformationMatrix transMat(mat);
        fn_joint.create();
        MDagPath dag_path;
        fn_joint.getPath(dag_path);
        data_.joints.append(dag_path);
        fn_joint.set(transMat);

        MString boneName(bone.name);
        fn_joint.setName(boneName, &status);
        if (status == MS::kFailure)
            MGlobal::displayWarning("SklReader: \"" + boneName + "\" bone name already in use in the world dag path, \n\
                                        the name will be replaced by maya to make it unique...");
    }

    // connect bones
    int data_num_bones = static_cast<int>(data_.num_bones);
    for (int i = 0; i < data_num_bones; i++)
    {
        SklBone bone = data_.bones.at(i);
        int parent = bone.parent;
        if (parent == i)
        {
            MString boneName(bone.name);
            MGlobal::displayWarning("SklReader: \"" + boneName + "\" this bone is telling me he is its own parent ... oO wtf");
        }
        else if (parent != -1)
        {
            MFnIkJoint fnParentJoint(data_.joints[parent]); // no need to check status
            MFnIkJoint fnChildJoint(data_.joints[i]);
            MVector pos = fnChildJoint.getTranslation(MSpace::kTransform);
            MQuaternion rot;
            fnChildJoint.getRotation(rot, MSpace::kWorld);
            MObject childJoint = fnChildJoint.object();
            fnParentJoint.addChild(childJoint);
            if (data_.version != 3)
            {
                fnChildJoint.setTranslation(pos, MSpace::kWorld);
                fnChildJoint.setRotation(rot, MSpace::kWorld);
            }
        }
    }

    return MS::kSuccess;
}

MStatus SklReader::templateUnused()
{
    if (data_.version == 2)
    {
        int num_influences = data_.num_indices;
        for (int i = 0; i < data_.num_bones; i++)
        {
            int j = 0;
            for (; j < num_influences; j++)
            {
                if (data_.skn_indices[j] == static_cast<int>(i))
                    break;
            }
            if (j != num_influences)
                continue;
            
            MFnIkJoint joint(data_.joints[i]);
            if (joint.childCount())
                continue;

            MString jointName = MFnIkJoint(data_.joints[i]).name();
            MGlobal::executeCommand("toggle -template -state on " + jointName);
        }
    }

    return MS::kSuccess;
}

} // namespace riot

