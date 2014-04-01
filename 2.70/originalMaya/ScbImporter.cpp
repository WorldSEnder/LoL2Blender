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

#include <ScbImporter.h>

#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MIOStream.h>
#include <maya/MFStream.h>

#include <maya_misc.h>

#include <ScbReader.h>

namespace riot {

void* ScbImporter::creator()
{
    return new ScbImporter();
}

MStatus ScbImporter::reader(const MFileObject& file, 
                         const MString& /*options*/, 
                         MPxFileTranslator::FileAccessMode mode) 
{
    if (MPxFileTranslator::kImportAccessMode != mode)
    {
        MGlobal::displayInfo("ScbImporter: only support \"import\"");
        return MStatus::kFailure;
    }

    #if defined (OSMac_)
        FAILURE("Sorry guys, I hate Apple.");
        /*
        char name_buffer[MAXPATHLEN];
        strcpy(name_buffer, file.fullName().asChar());
        const MString file_name(nameBuffer);
        */
    #else
        const MString file_name = file.fullName();
    #endif

    ifstream fin(file_name.asChar(), ios::binary);
    if (!fin)
        FAILURE("ScbImporter: " + file_name + " : could not be opened for reading");

    ScbReader *reader = new ScbReader();

    if (MStatus::kFailure == reader->read(fin))
    {
        delete reader;
        FAILURE("ScbImporter: reader->read(" + file_name + "); failed");
    }
    if (MStatus::kFailure == reader->loadData())
    {
        delete reader;
        FAILURE("ScbImporter: reader->loadData(); failed");
    }

    fin.close();
    delete reader;

    MGlobal::displayInfo("ScbImporter: import from " + file_name + " successful!");
    return MS::kSuccess;
}

MPxFileTranslator::MFileKind ScbImporter::identifyFile(
    const MFileObject& /*file*/, 
    const char* buffer, 
    short size) const
{
    if (size >= 152 && !_strnicmp(buffer, "r3d2Mesh", 8))
        return kIsMyFileType;

    return kNotMyFileType;
}

} // namespace riot

