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

#include <FixAnim.h>

#include <vector>

#include <maya/MFnIkJoint.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MQuaternion.h>
#include <maya/MVector.h>
#include <maya/MVectorArray.h>

#include <maya_misc.h>

namespace riot {

void* FixAnimCmd::creator()
{
    return new FixAnimCmd();
}

void FixAnimCmd::initialize()
{
    int result;    
    MGlobal::executeCommand("shelfLayout -q -ex Riot", result);
    if (!result)
        MGlobal::displayError("freezeRotCmd: Riot tab non-present oO");

    MStringArray buttons;
    MGlobal::executeCommand("shelfLayout -q -ca Riot", buttons);

    // delete previous buttons if they exist (search by command)
    int buttons_length = static_cast<int>(buttons.length());
    for (int i = 0; i < buttons_length; i++)
    {
        MString command;
        MGlobal::executeCommand("shelfButton -q -command " + buttons[i], command);
        if (!strncmp(command.asChar(), "fixAnim", 7))
            MGlobal::executeCommand("deleteUI " + buttons[i]);
    }

    // create buttons on the "Riot" shelf
    MString button;
    MGlobal::executeCommand("shelfButton \
            -p Riot \
            -command \"fixAnim\" \
            -annotation \"Fix animations of selected joints using the bindpose.\" \
            -label \"fixAnim\" \
            -i \"fixAnim.png\"", button);
    MGlobal::executeCommand("shelfLayout -e -position " + button + " 7 Riot");
}

MStatus FixAnimCmd::doIt(const MArgList& /*args*/)
{
    MFnIkJoint fn_joint;
    MDagPath dag_path;
    MDagPathArray joints;
    MVectorArray vectors;
    std::vector<MQuaternion> quaternions;
    MStatus status;

    MGlobal::displayInfo("Fixing selected joints ...");
    MSelectionList selection_list;
    if (MStatus::kSuccess != MGlobal::getActiveSelectionList(selection_list))
        FAILURE("fixAnimCmd: MGlobal::getActiveSelectionList()");

    MItSelectionList it_selection_list(selection_list, MFn::kJoint, &status);    
    if (status != MStatus::kSuccess)
        FAILURE("fixAnimCmd: it_selection_list()");

    MGlobal::executeCommand("gotoBindPose");

    for (; !it_selection_list.isDone(); it_selection_list.next())
    {
        it_selection_list.getDagPath(dag_path);
        fn_joint.setObject(dag_path);
        MQuaternion rotation;
        fn_joint.getRotation(rotation, MSpace::kTransform);
        MVector vec;
        vec = fn_joint.getTranslation(MSpace::kTransform);

        joints.append(dag_path);
        quaternions.push_back(rotation);
        vectors.append(vec);
    }

    int num_joints = joints.length();

    MString command("setKeyframe -breakdown 0 -hierarchy none -controlPoints 0 -shape 0\
                        -at translateX -at translateY -at translateZ\
                        -at rotateX -at rotateY -at rotateZ");

    int start, end;
    MGlobal::executeCommand("playbackOptions -q -animationStartTime", start);
    MGlobal::executeCommand("playbackOptions -q -animationEndTime", end);
    int num_frames = end - start + 1;

    MGlobal::executeCommand(MString("currentTime ") + start);

    for (int i = 0; i < num_joints; i++)
    {
            fn_joint.setObject(joints[i]);
            MQuaternion rotation;//, orient, axe;
            fn_joint.getRotation(rotation, MSpace::kTransform);
            //fn_joint.getOrientation(orient);
            //axe = fn_joint.rotateOrientation(MSpace::kTransform);
            //rotation = axe * rotation * orient;
            MVector vec;
            vec = fn_joint.getTranslation(MSpace::kTransform);

            // the offset will be added to fix.
            quaternions[i] = quaternions[i] * rotation.inverse();
            vectors[i] = vectors[i] - vec;
    }

    //rekeys all
    for (int i = 0; i < num_frames; i++)
    {
        MGlobal::executeCommand(MString("currentTime ") + (i + start));
        MGlobal::executeCommand(command);
    }

    for (int i = 0; i < num_frames; i++)
    {
        MGlobal::executeCommand(MString("currentTime ") + (i + start));

        for (int j = 0; j < num_joints; j++)
        {
            fn_joint.setObject(joints[j]);
            MQuaternion rotation;//, orient, axe;
            fn_joint.getRotation(rotation, MSpace::kTransform);
            //fn_joint.getOrientation(orient);
            //axe = fn_joint.rotateOrientation(MSpace::kTransform);
            //rotation = axe * rotation * orient;
            MVector vec;
            vec = fn_joint.getTranslation(MSpace::kTransform);

            rotation = quaternions[j] * rotation;
            vec += vectors[j];

            fn_joint.setRotation(rotation, MSpace::kTransform);
            fn_joint.setTranslation(vec, MSpace::kTransform);
        }
    }

    MGlobal::displayInfo("Selected joints fixed !");

    return MS::kSuccess;
}

} // namespace riot

