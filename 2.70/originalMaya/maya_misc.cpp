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

#include <maya_misc.h>

#include <maya/MIntArray.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MString.h>

namespace riot {

MPlug firstNotConnectedElement(MPlug& plug)
{
    MPlug ret_plug;
    MIntArray used_indices;
    plug.getExistingArrayAttributeIndices(used_indices);

    int i = 0;
    do
    {
        ret_plug = plug.elementByLogicalIndex(i);
        i++;
    } while (ret_plug.isConnected());

    return ret_plug;
}

void createTransButtons()
{
    int result;
    MGlobal::executeCommand("shelfLayout -q -ex Riot", result);
    if (!result)
        MGlobal::displayError("createTransButtons: Riot tab non-present oO");

    MStringArray buttons;
    MGlobal::executeCommand("shelfLayout -q -ca Riot", buttons);

    // delete previous buttons if they exist (search by command)
    int buttons_length = static_cast<int>(buttons.length());
    for (int i = 0; i < buttons_length; i++)
    {
        MString image;
        MGlobal::executeCommand("shelfButton -q -i " + buttons[i], image);
        if (!strncmp(image.asChar(), "std_", 4))
            MGlobal::executeCommand("deleteUI " + buttons[i]);
    }

    // create buttons on the "Riot" shelf
    MString button;
    MGlobal::executeCommand("shelfButton \
            -p Riot \
            -command \"Import\" \
            -annotation \"Import\" \
            -label \"Import\" \
            -i \"std_imp.png\"", button);
    MGlobal::executeCommand("shelfLayout -e -position " + button + " 1 Riot");
    MGlobal::executeCommand("shelfButton \
            -p Riot \
            -command \"Export\" \
            -annotation \"Export All\" \
            -label \"Export All\" \
            -i \"std_exp.png\"", button);
    MGlobal::executeCommand("shelfLayout -e -position " + button + " 2 Riot");
    MGlobal::executeCommand("shelfButton \
            -p Riot \
            -command \"ExportSelection\" \
            -annotation \"Export Selection\" \
            -label \"Export Selection\" \
            -i \"std_expsel.png\"", button);
    MGlobal::executeCommand("shelfLayout -e -position " + button + " 3 Riot");
}

void deleteRiotTab(void* /*clientData*/)
{
    MGlobal::executeCommand("deleteShelfTabNC Riot");
}

int hashName(const char* name)
{
    int i = 0;

    const char* c = name;
    for ( i = 0; *c; c++ )
    {
        i = tolower(*c) + 16 * i;
        if ( i & 0xF0000000 )
            i ^= i & 0xF0000000 ^ ((i & 0xF0000000u) >> 24);
    }

    return i;
}


} // namespace riot

