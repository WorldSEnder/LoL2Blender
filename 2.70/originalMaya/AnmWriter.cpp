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

#include <AnmWriter.h>

#include <maya/MGlobal.h>
#include <maya/MFnIkJoint.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MQuaternion.h>
#include <maya/MItDag.h>

#include <maya_misc.h>

namespace riot {

MStatus AnmWriter::write(ostream& file)
{
    data_.switchHand();

    // set magic
    char magic[9] = "r3d2anmd";
    file.write(magic, 8);

    // set version
    int version = 3;
    file.write(reinterpret_cast<char*>(&version), 4);

    // set designer ID
    int designer_id = 0x84211248;
    file.write(reinterpret_cast<char*>(&designer_id), 4);

    // set num_bones
    int num_bones = data_.num_bones;
    file.write(reinterpret_cast<char*>(&num_bones), 4);

    // set num_frames
    int num_frames = data_.num_frames;
    file.write(reinterpret_cast<char*>(&num_frames), 4);

    // set fps
    int fps = static_cast<int>(data_.fps);
    file.write(reinterpret_cast<char*>(&fps), 4);

    // set bones with frames
    for (int i = 0; i < num_bones; i++)
    {
        AnmBone bone = data_.bones.at(i);
        file.write(reinterpret_cast<char*>(&bone), AnmBone::kHeaderSize);
        for (int j = 0; j < num_frames; j++)
        {
            AnmPos pos = bone.poses.at(j);
            file.write(reinterpret_cast<char*>(&pos), AnmPos::kSizeInFile);
        }
    }

    return MS::kSuccess;
}

MStatus AnmWriter::dumpData()
{
    data_.version = 3;

    // get anim config
    double fps;
    MGlobal::executeCommand("playbackOptions -q -ps", fps);
    fps *= 24.0f;
    data_.fps = static_cast<float>(fps);

    int start, end;
    MGlobal::executeCommand("playbackOptions -q -animationStartTime", start);
    MGlobal::executeCommand("playbackOptions -q -animationEndTime", end);
    data_.num_frames = end - start + 1;

    MFnIkJoint fn_joint;
    MDagPath dag_path;
    MStatus status;

    MItDag it_dag(MItDag::kDepthFirst, MFn::kJoint, &status);
    if (status != MStatus::kSuccess)
    {
        FAILURE("AnmWriter: MItDag::MItDag()");
    }

    int num_joints = 0;
    for (; !it_dag.isDone(); it_dag.next())
    {
        AnmBone bone;
        num_joints++;
        it_dag.getPath(dag_path);
        data_.joints.append(dag_path);
        fn_joint.setObject(dag_path);
        MString joint_name = fn_joint.name();
        strcpy_s(bone.name, AnmBone::kNameLen, joint_name.asChar());
        data_.bones.push_back(bone);
    }
    data_.num_bones = num_joints;


    // flagin bones
    for (int i = 0; i < num_joints; i++)
    {
        fn_joint.setObject(data_.joints[i]);
        if (fn_joint.parentCount() == 1 && fn_joint.parent(0).apiType() == MFn::kJoint)
        {
            MFnIkJoint fn_parent_joint(fn_joint.parent(0));
            fn_parent_joint.getPath(dag_path);
            int j = 0;
            for (; j < num_joints; j++)
            {
                if (dag_path == data_.joints[j])
                {
                    data_.bones[i].flag = 0;
                    break;
                }
            }
            if (j == num_joints)
                FAILURE("AnmWriter: parent is not upper in hierarchy ? .. oO");
        }
        else
        {
            data_.bones[i].flag = 2; // no parent
        }
    }

    int num_bones = data_.num_bones;
    int num_frames = data_.num_frames;
    for (int i = 0; i < num_frames; i++)
    {
        MGlobal::executeCommand(MString("currentTime ") + (i + start));

        for (int j = 0; j < num_bones; j++)
        {
            AnmPos pos;
            fn_joint.setObject(data_.joints[j]);
            MQuaternion rotation, orient, axe;
            fn_joint.getRotation(rotation, MSpace::kTransform);
            fn_joint.getOrientation(orient);
            axe = fn_joint.rotateOrientation(MSpace::kTransform);
            rotation = axe * rotation * orient;
            MVector vec;
            vec = fn_joint.getTranslation(MSpace::kTransform);
            pos.rot[0] = static_cast<float>(rotation.x);
            pos.rot[1] = static_cast<float>(rotation.y);
            pos.rot[2] = static_cast<float>(rotation.z);
            pos.rot[3] = static_cast<float>(rotation.w);
            pos.x = static_cast<float>(vec.x);
            pos.y = static_cast<float>(vec.y);
            pos.z = static_cast<float>(vec.z);
            data_.bones[j].poses.push_back(pos);
        }
    }

    return MS::kSuccess;
}

} // namespace riot

