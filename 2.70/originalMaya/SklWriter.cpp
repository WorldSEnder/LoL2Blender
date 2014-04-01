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

#include <SklWriter.h>

#include <maya/MGlobal.h>
#include <maya/MFnIkJoint.h>
#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MMatrix.h>
#include <maya/MVector.h>
#include <maya/MDagPath.h>
#include <maya/MQuaternion.h>

#include <maya_misc.h>

namespace riot {

MStatus SklWriter::write(ostream& file)
{
    data_.switchHand();

    // magic
    char magic[9] = "r3d2sklt";
    file.write(magic, 8);

    // set version
    int version = data_.version; // != 3
    file.write(reinterpret_cast<char*>(&version), 4);
    if (version != 2 && version != 1)
        FAILURE("SklWriter: skl type not supported, \n please report that to ThiSpawn");

    // set designer ID
    int designerId = 0x84211248;
    file.write(reinterpret_cast<char*>(&designerId), 4);

    // set num_bones
    int num_bones = data_.num_bones;
    file.write(reinterpret_cast<char*>(&num_bones), 4);

    // set bones
    for (int i = 0; i < num_bones; i++)
    {
        SklBone bone = data_.bones[i];
        file.write(reinterpret_cast<char*>(&bone), SklBone::kSizeWithoutMatrix);
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 4; k++)
                file.write(reinterpret_cast<char*>(&(bone.transform[k][j])), 4);
    }

    // set end tab
    if (version == 2)
    {
        int num_indices = data_.num_indices;
        file.write(reinterpret_cast<char*>(&num_indices), 4);
        for (int i = 0; i < num_indices; i++)
        {
            int sknID = data_.skn_indices[i];
            file.write(reinterpret_cast<char*>(&sknID), 4);
        }
    }

    return MS::kSuccess;
}

MStatus SklWriter::dumpData()
{
    MStatus status;
    MDagPath dag_path;
    MFnIkJoint fn_joint;

    // MItDag::kDepthFirst used to assure hierarchical order :)
    MItDag it_dag(MItDag::kDepthFirst, MFn::kJoint, &status);
    if (status != MStatus::kSuccess)
        FAILURE("SklWriter: MItDag::MItDag()");

    int num_joints = 0;
    for (; !it_dag.isDone(); it_dag.next())
    {
        SklBone bone;
        num_joints++;
        it_dag.getPath(dag_path);
        data_.joints.append(dag_path);
        fn_joint.setObject(dag_path);
        MString jointName = fn_joint.name();
        strcpy_s(bone.name, SklBone::kNameLen, jointName.asChar());
        MQuaternion rotation, axe;
        axe = fn_joint.rotateOrientation(MSpace::kTransform);
        fn_joint.getRotation(rotation, MSpace::kWorld); // since it's kWorld it will get orientation too.
        rotation = axe * rotation; // but care the devil in the details :)
        MVector translation = fn_joint.getTranslation(MSpace::kWorld);
        MTransformationMatrix transform;
        transform.setRotationQuaternion(rotation.x, rotation.y, rotation.z, rotation.w, MSpace::kWorld);
        transform.setTranslation(translation, MSpace::kWorld);
        MMatrix mat = transform.asMatrix();
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                bone.transform[j][k] = static_cast<float>(mat[j][k]);
        data_.bones.push_back(bone);
    }
    data_.num_bones = num_joints;

    // parenting bones
    for (int i = 0; i < num_joints; i++)
    {
        fn_joint.setObject(data_.joints[i]);
        if (fn_joint.parentCount() == 1 && fn_joint.parent(0).apiType() == MFn::kJoint)
        {
            MFnIkJoint fnParentJoint(fn_joint.parent(0));
            fnParentJoint.getPath(dag_path);
            int j = 0;
            for (; j < num_joints; j++)
            {
                if (dag_path == data_.joints[j])
                {
                    data_.bones[i].parent = j;
                    break;
                }
            }
            if (j == num_joints)
                FAILURE("SklWriter: parent is not upper in hierarchy ? .. oO");
        }
        else
        {
            data_.bones[i].parent = -1;
        }
    }

    return MS::kSuccess;
}

} // namespace riot

