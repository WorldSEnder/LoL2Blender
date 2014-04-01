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

#include <ResetBindPose.h>

#include <maya/MFnIkJoint.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MStringArray.h>

#include <maya_misc.h>

namespace riot {

void* ResetBindPoseCmd::creator()
{
    return new ResetBindPoseCmd();
}

void ResetBindPoseCmd::initialize()
{
    int result;    
    MGlobal::executeCommand("shelfLayout -q -ex Riot", result);
    if (!result)
        MGlobal::displayError("resetBindPoseCmd: Riot tab non-present oO");

    MStringArray buttons;
    MGlobal::executeCommand("shelfLayout -q -ca Riot", buttons);

    // delete previous buttons if they exist (search by command)
    int buttons_length = static_cast<int>(buttons.length());
    for (int i = 0; i < buttons_length; i++)
    {
        MString command;
        MGlobal::executeCommand("shelfButton -q -command " + buttons[i], command);
        if (!strncmp(command.asChar(), "resetBindPose", 13))
            MGlobal::executeCommand("deleteUI " + buttons[i]);
    }

    // create buttons on the "Riot" shelf
    MString button;
    MGlobal::executeCommand("shelfButton \
            -p Riot \
            -command \"resetBindPose\" \
            -annotation \"Reset bindPose of selected skinned mesh with current pose.\" \
            -label \"resetBindPose\" \
            -i \"resetBindPose.png\"", button);
    MGlobal::executeCommand("shelfLayout -e -position " + button + " 6 Riot");
}

MStatus ResetBindPoseCmd::doIt(const MArgList& /*args*/)
{
    MStatus status;
    MDagPath dag_path;
    MDagPath mesh_dag_path;

    MSelectionList selection_list;
    if (MStatus::kSuccess != MGlobal::getActiveSelectionList(selection_list))
        FAILURE("resetBindPoseCmd: MGlobal::getActiveSelectionList()");

    MItSelectionList it_selection_list(selection_list, MFn::kMesh, &status);    
    if (status != MStatus::kSuccess)
        FAILURE("resetBindPoseCmd: it_selection_list()");

    if (it_selection_list.isDone())
        FAILURE("resetBindPoseCmd: no mesh selected!");

    it_selection_list.getDagPath(mesh_dag_path);
    it_selection_list.next();

    if (!it_selection_list.isDone())
        FAILURE("resetBindPoseCmd: more than one mesh selected!");

    MFnMesh mesh(mesh_dag_path);

    MPlug in_mesh_plug = mesh.findPlug("inMesh");
    MPlugArray in_mesh_connections;
    in_mesh_plug.connectedTo(in_mesh_connections, true, false);
    if (in_mesh_connections.length() == 0)
        FAILURE("resetBindPoseCmd: failed to find the mesh's skinCluster.");
    MPlug output_geom_plug = in_mesh_connections[0];
    MFnSkinCluster fn_skin_cluster(output_geom_plug.node());

    MPlug bind_pose_plug = fn_skin_cluster.findPlug("bindPose");
    MPlugArray bind_pose_connections;
    bind_pose_plug.connectedTo(bind_pose_connections, true, false);
    if (bind_pose_connections.length() == 0)
        FAILURE("resetBindPoseCmd: failed to find the skinCluster's bindPose.");
    MPlug message_plug = bind_pose_connections[0];
    MFnDependencyNode fn_bind_pose_dep_node(message_plug.node());

    MString bind_pose_name = fn_bind_pose_dep_node.name();
    MString command("dagPose -reset " + bind_pose_name);

    MStringArray joints;
    MGlobal::executeCommand("dagPose -q -m " + bind_pose_name, joints);
    int joints_length = static_cast<int>(joints.length());
    for (int i = 0; i < joints_length; i++)
        command += " " + joints[i];

    MGlobal::executeCommand(command);

    MGlobal::displayInfo("ResetBindPose successful !");

    return MS::kSuccess;
}

} // namespace riot

