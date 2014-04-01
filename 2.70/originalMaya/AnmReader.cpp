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

#include <AnmReader.h>

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

MStatus AnmReader::read(istream& file)
{
    // get length
    int minlen = 28;
    file.seekg (0, ios::end);
    int length = file.tellg();
    file.seekg (0, ios::beg);

    // check minimum length
    if (length < minlen)
        FAILURE("AnmReader: the file is empty!");

    // check magic
    char magic[8];
    file.read(magic, 8);
    if (strncmp(magic, "r3d2anmd", 8))
        FAILURE("AnmReader: magic is wrong!");

    // get version
    int version;
    file.read(reinterpret_cast<char*>(&version), 4);
    if (version == 3)
    {
        data_.version = version;

        // get designer ID
        int designer_id;
        file.read(reinterpret_cast<char*>(&designer_id), 4);

        // get num_bones
        int num_bones;
        file.read(reinterpret_cast<char*>(&num_bones), 4);

        // get num_frames
        int num_frames;
        file.read(reinterpret_cast<char*>(&num_frames), 4);

        // get fps (algorithm seen during the reversing)
        float fps;
        float ffps;
        file.read(reinterpret_cast<char*>(&ffps), 4);
        if (ffps < 0.0f)
            fps = ffps + 4294967296.0f;
        else
            fps = ffps;
        data_.fps = fps;

        // check minimum length
        minlen += num_bones * num_frames * AnmPos::kSizeInFile + num_bones * AnmBone::kHeaderSize;
        if (length < minlen)
            FAILURE("AnmReader: unexpected end of file");
        data_.num_bones = num_bones;
        data_.num_frames = num_frames;

        // get bones with frames
        for (int i = 0; i < num_bones; i++)
        {
            AnmBone bone;
            file.read(reinterpret_cast<char*>(&bone), AnmBone::kHeaderSize);
            for (int j = 0; j < num_frames; j++)
            {
                AnmPos pos;
                file.read(reinterpret_cast<char*>(&pos), AnmPos::kSizeInFile);
                bone.poses.push_back(pos);
            }
            data_.bones.push_back(bone);
        }

        data_.switchHand();
    }
    else if (version == 4)
    {
        MGlobal::displayInfo("AnmReader: anm is of version 4, this support is in beta test, report any problems.");

        data_.version = version;

        // get data size
        int data_size;
        file.read(reinterpret_cast<char*>(&data_size), 4);

        minlen = 12 + data_size;
        if (length < minlen)
            FAILURE("AnmReader: unexpected end of file");

        // get magic
        int magic;
        file.read(reinterpret_cast<char*>(&magic), 4);

        if (magic != 0xBE0794D3)
            FAILURE("AnmReader: v4, magic is wrong!");

        // 2 bytes unused
        file.seekg(8, ios::cur);

        // get num_bones
        int num_bones;
        file.read(reinterpret_cast<char*>(&num_bones), 4);

        // get num_frames
        int num_frames;
        file.read(reinterpret_cast<char*>(&num_frames), 4);

        // get fps (algorithm seen during the reversing)
        float fps;
        float ffps;
        file.read(reinterpret_cast<char*>(&ffps), 4);
        if (ffps < 1.0f)
            fps = 1.0f / ffps;
        else
            fps = ffps;
        data_.fps = fps;
       
        data_.num_bones = num_bones;
        data_.num_frames = num_frames;

        // 3 bytes unused
        file.seekg(12, ios::cur);

        int positions_offset;
        file.read(reinterpret_cast<char*>(&positions_offset), 4);
        
        int quaternions_offset;
        file.read(reinterpret_cast<char*>(&quaternions_offset), 4);

        int frames_offset;
        file.read(reinterpret_cast<char*>(&frames_offset), 4);

        int num_pos = (quaternions_offset - positions_offset) / 12;
        int num_quat = (frames_offset - quaternions_offset) / 16;

        // 3 bytes unused
        file.seekg(12, ios::cur);

        // fill positions vector
        Vec3* positions = reinterpret_cast<Vec3*>(malloc(num_pos * sizeof(Vec3)));
        file.read(reinterpret_cast<char*>(positions), num_pos * sizeof(Vec3));

        // fill quaternions vector
        Quat* quaternions = reinterpret_cast<Quat*>(malloc(num_quat * sizeof(Quat)));
        file.read(reinterpret_cast<char*>(quaternions), num_quat * sizeof(Quat));

        // get bones with frames
        
        for (int i = 0; i < num_bones; i++)
        {
            AnmBone bone;
            data_.bones.push_back(bone);
        }

        for (int i = 0; i < num_frames; i++)
        {
            for (int j = 0; j < num_bones; j++)
            {
                int name_hash;
                file.read(reinterpret_cast<char*>(&name_hash), 4);
                WORD pos_id;
                file.read(reinterpret_cast<char*>(&pos_id), 2);
                file.seekg(2, ios::cur); // pos id from unit pos, useless for us
                WORD quat_id;
                file.read(reinterpret_cast<char*>(&quat_id), 2);
                file.seekg(2, ios::cur); // 0

                if (i == 0)
                    data_.bones.at(j).name_hash = name_hash;

                AnmPos pos;
                memcpy(&(pos.x), positions + pos_id, sizeof(Vec3));
                memcpy(pos.rot, quaternions + quat_id, sizeof(Quat));

                data_.bones.at(j).poses.push_back(pos);
            }
        }

        free(positions);
        free(quaternions);

        data_.switchHand();
    }
    else
    {
        FAILURE("AnmReader: anm type not supported, \n please report that to ThiSpawn");
    }

    return MS::kSuccess;
}

MStatus AnmReader::loadData()
{
    // the bones don't need to be in hierarchical order
    // prevent update for later type versions.

    MFnIkJoint fn_joint;
    
    MStatus status;
    MDagPath dag_path;

    data_.joints.clear();

    MString command("setKeyframe -breakdown 0 -hierarchy none -controlPoints 0 -shape 0" \
                    "-at translateX -at translateY -at translateZ" \
                    "-at rotateX -at rotateY -at rotateZ");

    // get paths
    for (int i = 0; i < data_.num_bones; i++)
    {
        if (data_.bones[i].name_hash != 0) // for version 4
        {
            MItDag it_dag(MItDag::kDepthFirst, MFn::kJoint, &status);
            if (status != MStatus::kSuccess)
                FAILURE("AnmReader: MItDag::MItDag()");

            int tested_hash;
            int name_hash = data_.bones[i].name_hash;
            MString joint_name;

            for (; !it_dag.isDone(); it_dag.next())
            {
                it_dag.getPath(dag_path);
                MFnIkJoint joint(dag_path);
                joint_name = joint.name();
                tested_hash = hashName(joint_name.asChar());
                if (name_hash == tested_hash)
                {
                    data_.joints.append(dag_path);
                    break;
                }
            }

            if (it_dag.isDone())
            {
                MGlobal::displayWarning(MString("AnmReader: no bone with name hash (int)")
                                        + name_hash
                                        + " found.");
                data_.bones.erase(data_.bones.begin() + i);
                data_.num_bones -= 1;
                i--;
            }
            else
            {
                command += " " + joint_name;
            }
        }
        else
        {
            MString bone_name(data_.bones[i].name);
            status = MGlobal::selectByName(bone_name, MGlobal::ListAdjustment::kReplaceList);
            if (status == MS::kSuccess)
            {
                MSelectionList active_list;
                MGlobal::getActiveSelectionList(active_list);
                if (active_list.length() != 1)
                    FAILURE("AnmReader: too much bones named : " + bone_name);

                MItSelectionList iter(active_list, MFn::kJoint);

                // only one object is selected
                if (iter.isDone())
                {
                    MGlobal::displayWarning("AnmReader: " + bone_name + " is not a joint ...");
                    continue;
                }

                iter.getDagPath(dag_path);
                data_.joints.append(dag_path);
                MFnIkJoint joint(dag_path);
                command += " " + joint.name();
            }
            else // try to find the bone without the maya case sensitive way
            {
                MItDag it_dag(MItDag::kDepthFirst, MFn::kJoint, &status);
                if (status != MStatus::kSuccess)
                    FAILURE("AnmReader: MItDag::MItDag()");

                MString low_bone_name = bone_name;
                low_bone_name.toLowerCase();
                MString joint_name;

                for (; !it_dag.isDone(); it_dag.next())
                {
                    it_dag.getPath(dag_path);
                    MFnIkJoint joint(dag_path);
                    MString low_joint_name = joint.name();
                    low_joint_name.toLowerCase();
                    if (low_bone_name == low_joint_name)
                    {
                        data_.joints.append(dag_path);
                        joint_name = joint.name();
                        break;
                    }
                }

                if (it_dag.isDone())
                {
                    MGlobal::displayWarning("AnmReader: no bone named " + MString(data_.bones[i].name) + " found.");
                    data_.bones.erase(data_.bones.begin() + i);
                    data_.num_bones -= 1;
                    i--;
                }
                else
                {
                    command += " " + joint_name;
                }
            }
        }
    }

    if (data_.num_bones <= 1)
        MGlobal::displayWarning("AnmReader: erm .. is this an anm for this champ ??? oO");

    // set anim config
    MString playback_options("playbackOptions -e");
    playback_options += " -min ";
    playback_options += 0;
    playback_options += " -max ";
    playback_options += data_.num_frames - 1;
    playback_options += " -animationStartTime ";
    playback_options += 0;
    playback_options += " -animationEndTime ";
    playback_options += data_.num_frames - 1;
    playback_options += " -playbackSpeed ";
    playback_options += (data_.fps / 24.0f);

    MGlobal::executeCommand(playback_options);

    int num_bones = data_.num_bones;
    int num_frames = data_.num_frames;
    for (int i = 0; i < num_frames; i++)
    {
        MGlobal::executeCommand(MString("currentTime ") + i);
        /*
        The transformation matrix for a joint node is below.

            matrix = [S] * [RO] * [R] * [JO] * [IS] * [T]

        (where '*' denotes matrix multiplication).

        These matrices are defined as follows:

            [S] : scale
            [RO] : rotateOrient (attribute name is rotateAxis)
            [R] : rotate
            [JO] : jointOrient
            [IS] : parentScaleInverse
            [T] : translate
        */
        for (int j = 0; j < num_bones; j++)
        {
            AnmPos pos = data_.bones[j].poses[i];
            fn_joint.setObject(data_.joints[j]);
            MQuaternion rotation(pos.rot[0], pos.rot[1], pos.rot[2], pos.rot[3]);
            MQuaternion orient, axe;
            fn_joint.getOrientation(orient);
            axe = fn_joint.rotateOrientation(MSpace::kTransform);
            rotation = axe.inverse() * rotation * orient.inverse();
            fn_joint.setRotation(rotation, MSpace::kTransform);
            MVector vec(pos.x, pos.y, pos.z);
            fn_joint.setTranslation(vec, MSpace::kTransform);
        }

        MGlobal::executeCommand(command);
    }

    return MS::kSuccess;
}

} // namespace riot

