/*
    Copyright 2011 (ThiSpawn on gamedeception.com).

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

#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MSceneMessage.h>

#include <SklImporter.h>
#include <SknImporter.h>
#include <AnmImporter.h>
#include <ScoImporter.h>
#include <ScbImporter.h>
#include <SkExporter.h>
#include <AnmExporter.h>
#include <ScoExporter.h>
#include <ScbExporter.h>
#include <freezeRot.h>
#include <resetBindPose.h>
#include <fixAnim.h>
#include <maya_misc.h>

MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, "ThiSpawn", "1.0");

    MSceneMessage::addCallback(MSceneMessage::kMayaExiting, riot::deleteRiotTab);

    // import
    status = plugin.registerFileTranslator("League of Legends - skeleton only", "", riot::SklImporter::creator, "RiotFileSklImportOptions", "", true);
    if (!status)
    {
        status.perror("registerFileTranslator(\"League of Legends - skeleton only\"..");
        return status;
    }
    status = plugin.registerFileTranslator("League of Legends - skin", "", riot::SknImporter::creator, "RiotFileSknImportOptions", "", true);
    if (!status)
    {
        status.perror("registerFileTranslator(\"League of Legends - skin\"..");
        return status;
    }
    status = plugin.registerFileTranslator("League of Legends - animation", "", riot::AnmImporter::creator, NULL, NULL, true);
    if (!status)
    {
        status.perror("registerFileTranslator(\"League of Legends - animation\"..");
        return status;
    }
    status = plugin.registerFileTranslator("League of Legends - sco", "", riot::ScoImporter::creator, NULL, NULL, true);
    if (!status)
    {
        status.perror("registerFileTranslator(\"League of Legends - sco\"..");
        return status;
    }
    status = plugin.registerFileTranslator("League of Legends - scb", "", riot::ScbImporter::creator, NULL, NULL, false);
    if (!status)
    {
        status.perror("registerFileTranslator(\"League of Legends - scb\"..");
        return status;
    }

    // export
    status = plugin.registerFileTranslator("League of Legends - export skin", "", riot::SkExporter::creator, NULL, NULL, false);
    if (!status)
    {
        status.perror("registerFileTranslator(\"League of Legends - export skin\"..");
        return status;
    }
    status = plugin.registerFileTranslator("League of Legends - export anim", "", riot::AnmExporter::creator, NULL, NULL, true);
    if (!status)
    {
        status.perror("registerFileTranslator(\"League of Legends - export anim\"..");
        return status;
    }
    status = plugin.registerFileTranslator("League of Legends - export sco", "", riot::ScoExporter::creator, NULL, NULL, true);
    if (!status)
    {
        status.perror("registerFileTranslator(\"League of Legends - sco\"..");
        return status;
    }
    status = plugin.registerFileTranslator("League of Legends - export scb", "", riot::ScbExporter::creator, NULL, NULL, false);
    if (!status)
    {
        status.perror("registerFileTranslator(\"League of Legends - scb\"..");
        return status;
    }

        

    // commands
    int result;    
    MGlobal::executeCommand("shelfLayout -q -ex Riot", result);
    if (!result)
        MGlobal::executeCommand("addNewShelfTab Riot");

    riot::createTransButtons();

    status = plugin.registerCommand("freezeRot", riot::FreezeRotCmd::creator);
    if (!status)
    {
        status.perror("registerCommand(\"freezeRot\"..");
        return status;
    }
    riot::FreezeRotCmd::initialize();
    status = plugin.registerCommand("resetBindPose", riot::ResetBindPoseCmd::creator);
    if (!status)
    {
        status.perror("registerCommand(\"resetBindPose\"..");
        return status;
    }
    riot::ResetBindPoseCmd::initialize();
    status = plugin.registerCommand("fixAnim", riot::FixAnimCmd::creator);
    if (!status)
    {
        status.perror("registerCommand(\"fixAnim\"..");
        return status;
    }
    riot::FixAnimCmd::initialize();

    //MGlobal::executeCommand("shelfLayout -e -cellHeight 35 Riot");
    //MGlobal::executeCommand("shelfLayout -e -cellWidth 35 Riot");
    //MGlobal::executeCommand("shelfLayout -e -height 42 Riot");

    return status;
}


MStatus uninitializePlugin(MObject obj) 
{
    MStatus   status;
    MFnPlugin plugin( obj );

    // import
    status =  plugin.deregisterFileTranslator("League of Legends - skeleton only");
    if (!status)
    {
        status.perror("deregisterFileTranslator(\"League of Legends - skeleton only\"..");
        return status;
    }
    status =  plugin.deregisterFileTranslator("League of Legends - skin");
    if (!status)
    {
        status.perror("deregisterFileTranslator(\"League of Legends - skin\"..");
        return status;
    }
    status =  plugin.deregisterFileTranslator("League of Legends - animation");
    if (!status)
    {
        status.perror("deregisterFileTranslator(\"League of Legends - animation\"..");
        return status;
    }
    status =  plugin.deregisterFileTranslator("League of Legends - sco");
    if (!status)
    {
        status.perror("deregisterFileTranslator(\"League of Legends - sco\"..");
        return status;
    }
    status =  plugin.deregisterFileTranslator("League of Legends - scb");
    if (!status)
    {
        status.perror("deregisterFileTranslator(\"League of Legends - scb\"..");
        return status;
    }

    // export
    status =  plugin.deregisterFileTranslator("League of Legends - export skin");
    if (!status)
    {
        status.perror("deregisterFileTranslator(\"League of Legends - export skin\"..");
        return status;
    }
    status =  plugin.deregisterFileTranslator("League of Legends - export anim");
    if (!status)
    {
        status.perror("deregisterFileTranslator(\"League of Legends - export animation\"..");
        return status;
    }
    status =  plugin.deregisterFileTranslator("League of Legends - export sco");
    if (!status)
    {
        status.perror("deregisterFileTranslator(\"League of Legends - export sco\"..");
        return status;
    }
    status =  plugin.deregisterFileTranslator("League of Legends - export scb");
    if (!status)
    {
        status.perror("deregisterFileTranslator(\"League of Legends - export scb\"..");
        return status;
    }


    // commands
    status =  plugin.deregisterCommand("freezeRot");
    if (!status)
    {
        status.perror("deregisterCommand(\"freezeRot\")");
        return status;
    }
    status =  plugin.deregisterCommand("resetBindPose");
    if (!status)
    {
        status.perror("deregisterCommand(\"resetBindPose\")");
        return status;
    }
    status =  plugin.deregisterCommand("fixAnim");
    if (!status)
    {
        status.perror("deregisterCommand(\"fixAnim\")");
        return status;
    }

    MGlobal::executeCommand("deleteShelfTabNC Riot");

    return status;
}
