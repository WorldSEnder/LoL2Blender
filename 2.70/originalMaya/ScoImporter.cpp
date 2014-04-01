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

#include <ScoImporter.h>

#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MIOStream.h>
#include <maya/MFStream.h>

#include <maya_misc.h>

#include <ScoReader.h>

namespace riot {

void* ScoImporter::creator()
{
    return new ScoImporter();
}

MStatus ScoImporter::reader(const MFileObject& file, 
                         const MString& /*options*/, 
                         MPxFileTranslator::FileAccessMode mode) 
{
    if (MPxFileTranslator::kImportAccessMode != mode)
    {
        MGlobal::displayInfo("ScoImporter: only support \"import\"");
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

    ifstream finSco(file_name.asChar()); // not binary
    if (!finSco)
        FAILURE("ScoImporter: " + file_name + " : could not be opened for reading");

    ScoReader *reader = new ScoReader();

    if (MStatus::kFailure == reader->read(finSco))
    {
        delete reader;
        FAILURE("ScoImporter: reader->read(" + file_name + "); failed");
    }
    if (MStatus::kFailure == reader->loadData())
    {
        delete reader;
        FAILURE("ScoImporter: reader->loadData(); failed");
    }

    finSco.close();
    delete reader;

    MGlobal::displayInfo("ScoImporter: import from " + file_name + " successful!");
    return MS::kSuccess;
}

MPxFileTranslator::MFileKind ScoImporter::identifyFile(
    const MFileObject& /*file*/, 
    const char* buffer, 
    short size) const
{
    if (size >= 13 && !_strnicmp(buffer, "[ObjectBegin]", 13))
    {
        return kIsMyFileType;
    }

    return    kNotMyFileType;
}

} // namespace riot

