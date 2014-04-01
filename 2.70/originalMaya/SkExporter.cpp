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

// skn export type will be 1
// cause 0 seems weird
// and dunno what the last three 0 mean in type 2

#include <SkExporter.h>

#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MIOStream.h>
#include <maya/MFStream.h>
#include <maya/MItDag.h>

#include <SklWriter.h>
#include <SknWriter.h>
#include <maya_misc.h>

namespace riot {

void* SkExporter::creator()
{
    return new SkExporter();
}

MStatus SkExporter::writer(const MFileObject& file, 
    const MString& /*options*/, 
    MPxFileTranslator::FileAccessMode mode) 
{
    if (MPxFileTranslator::kExportActiveAccessMode != mode)
    {
        MGlobal::displayInfo("sk::Exporter: only support \"export selected\"");
        return MStatus::kFailure;
    }

    #if defined (OSMac_)
        FAILURE("Sorry guys, I hate Apple.");
        /*
        char name_buffer[MAXPATHLEN];
        strcpy(name_buffer, file.fullName().asChar());
        const MString skn_file_name(nameBuffer);
        */
    #else
        const MString skn_file_name = file.fullName();
    #endif

    MString file_base_name = file.name();
    int rindex = file_base_name.rindexW('.');
    if (rindex != -1)
        file_base_name = file_base_name.substringW(0, rindex - 1);

    MString file_full_base_name = file.fullName();
    rindex = file_full_base_name.rindexW('.');
    if (rindex != -1)
        file_full_base_name = file_full_base_name.substringW(0, rindex - 1);
    const MString skl_file_name = file_full_base_name + ".skl";

    ofstream fout_skn(skn_file_name.asChar(), ios::binary);
    if (!fout_skn)
        FAILURE("sk::Exporter: " + skn_file_name + " : could not be opened for writing");

    ofstream fout_skl(skl_file_name.asChar(), ios::binary);
    if (!fout_skl)
        FAILURE("sk::Exporter: " + skl_file_name + " : could not be opened for writing");

    SknWriter *skn_writer = new SknWriter();
    SklWriter *skl_writer = new SklWriter();
    
    SklData *skl_data = &(skl_writer->data_);

    // dump me that !
    if (MStatus::kFailure == skl_writer->dumpData())
    {
        delete skn_writer;
        delete skl_writer;
        FAILURE("sk::Exporter: skl_writer->dumpData(); failed");
    }

    if (MStatus::kFailure == skn_writer->dumpData(skl_data))
    {
        delete skn_writer;
        delete skl_writer;
        FAILURE("sk::Exporter: skn_writer->dumpData(); failed");
    }

    // it's time to write the stuff;
    if (MStatus::kFailure == skn_writer->write(fout_skn))
    {
        delete skn_writer;
        delete skl_writer;
        FAILURE("sk::Exporter: skn_writer->write(" + skn_file_name + "); failed");
    }
    if (MStatus::kFailure == skl_writer->write(fout_skl))
    {
        delete skn_writer;
        delete skl_writer;
        FAILURE("sk::Exporter: skl_writer->write(" + skl_file_name + "); failed");
    }


    fout_skn.flush();
    fout_skn.close();
    delete skn_writer;

    fout_skl.flush();
    fout_skl.close();
    delete skl_writer;

    MGlobal::displayInfo("sk::Exporter: export successful!");
    return MS::kSuccess;
}

} // namespace riot

