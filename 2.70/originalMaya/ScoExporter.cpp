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

#include <ScoExporter.h>

#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MIOStream.h>
#include <maya/MFStream.h>

#include <maya_misc.h>

#include <ScoWriter.h>

namespace riot {

void* ScoExporter::creator()
{
    return new ScoExporter();
}

MStatus ScoExporter::writer(const MFileObject& file, 
                         const MString& /*options*/, 
                         MPxFileTranslator::FileAccessMode mode) 
{
    if (MPxFileTranslator::kExportActiveAccessMode != mode)
    {
        MGlobal::displayInfo("ScoExporter: only support \"export selected\"");
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

    ofstream fout(file_name.asChar()); // not a binary

    if (!fout)
        FAILURE("ScoExporter: " + file_name + " : could not be opened for reading");

    ScoWriter *writer = new ScoWriter();

    if (MStatus::kFailure == writer->dumpData())
    {
        delete writer;
        FAILURE("ScoExporter: writer->dumpData(): failed");
    }
    if (MStatus::kFailure == writer->write(fout))
    {
        delete writer;
        FAILURE("ScoExporter: writer->write(" + file_name + "); failed");
    }

    fout.flush();
    fout.close();
    delete writer;

    MGlobal::displayInfo("ScoExporter: export successful!");
    return MS::kSuccess;
}

} // namespace riot

