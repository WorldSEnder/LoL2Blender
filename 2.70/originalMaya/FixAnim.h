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
#ifndef RIOT__MISCFIXANIM_H
#define RIOT__MISCFIXANIM_H

#include <maya/MArgList.h>
#include <maya/MPxCommand.h>
#include <maya/MGlobal.h>

namespace riot {

class FixAnimCmd : public MPxCommand
{
public:
    static void* creator();
    bool isUndoable() const { return false; }
        
    static void initialize();
    MStatus doIt(const MArgList&);
};

} // namespace riot

#endif

