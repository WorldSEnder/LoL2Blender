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

#include <FreezeRot.h>

#include <maya/MFnIkJoint.h>
#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MItDag.h>
#include <maya/MQuaternion.h>

#include <maya_misc.h>

namespace riot {

void* FreezeRotCmd::creator()
{
    return new FreezeRotCmd();
}

void FreezeRotCmd::initialize()
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
        if (!strncmp(command.asChar(), "freezeRot", 9))
            MGlobal::executeCommand("deleteUI " + buttons[i]);
    }

    // create buttons on the "Riot" shelf
    MString button;
    MGlobal::executeCommand("shelfButton \
            -p Riot \
            -command \"freezeRot selected\" \
            -annotation \"Freeze rotations of selected joints.\" \
            -label \"freezeRot selected\" \
            -i \"freezeRotSel.png\"", button);
    MGlobal::executeCommand("shelfLayout -e -position " + button + " 4 Riot");
    MGlobal::executeCommand("shelfButton \
            -p Riot \
            -command \"freezeRot\" \
            -annotation \"Freeze rotations of all joints.\" \
            -label \"freezeRot\" \
            -i \"freezeRot.png\"", button);
    MGlobal::executeCommand("shelfLayout -e -position " + button + " 5 Riot");
}

MStatus FreezeRotCmd::doIt(const MArgList& args)
{
    bool is_sel_only = false;
    int num_args = static_cast<int>(args.length());
    for (int i = 0; i < num_args && !is_sel_only; i++)
    {
        MString string;
        args.get(i, string);
        if (string.indexW("selected") != -1)
            is_sel_only = true;
    }

    MFnIkJoint fn_joint;
    MDagPath dag_path;
    MStatus status;

    if (is_sel_only)
    {
        MGlobal::displayInfo("Freezing selected joints ...");
        MSelectionList selection_list;
        if (MStatus::kSuccess != MGlobal::getActiveSelectionList(selection_list))
            FAILURE("freezeRotCmd: MGlobal::getActiveSelectionList()");

        MItSelectionList it_selection_list(selection_list, MFn::kJoint, &status);    
        if (status != MStatus::kSuccess)
            FAILURE("freezeRotCmd: it_selection_list()");

        for (; !it_selection_list.isDone(); it_selection_list.next())
        {
            it_selection_list.getDagPath(dag_path);
            fn_joint.setObject(dag_path);
            MQuaternion orient, rot, axe;
            fn_joint.getOrientation(orient);
            fn_joint.getRotation(rot, MSpace::kTransform);
            orient = rot * orient;
            fn_joint.setOrientation(orient);
            fn_joint.setRotation(orient.identity);
        }

        MGlobal::displayInfo("Selected joints frozen !");
    }
    else
    {
        MGlobal::displayInfo("Freezing all joints ...");
        MItDag it_dag(MItDag::kDepthFirst, MFn::kJoint, &status);
        if (status != MStatus::kSuccess)
        {
            FAILURE("freezeRotCmd: MItDag::MItDag()");
        }

        for (; !it_dag.isDone(); it_dag.next())
        {
            it_dag.getPath(dag_path);
            fn_joint.setObject(dag_path);
            MQuaternion orient, rot, axe;
            fn_joint.getOrientation(orient);
            fn_joint.getRotation(rot, MSpace::kTransform);
            orient = rot * orient;
            fn_joint.setOrientation(orient);
            fn_joint.setRotation(orient.identity);
        }

        MGlobal::displayInfo("All joints frozen !");
    }

    return MS::kSuccess;
}

} // namespace riot

