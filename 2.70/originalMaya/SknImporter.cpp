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

#include <SknImporter.h>

#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MIOStream.h>
#include <maya/MFStream.h>

#include <SknReader.h>
#include <SklReader.h>
#include <maya_misc.h>

namespace riot {

void* SknImporter::creator()
{
    return new SknImporter();
}

MStatus SknImporter::reader(const MFileObject& file, 
                         const MString& options, 
                         MPxFileTranslator::FileAccessMode mode) 
{
    if (MPxFileTranslator::kImportAccessMode != mode)
    {
        MGlobal::displayInfo("SknImporter: only support \"import\"");
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

    MStringArray option_list;
    MStringArray the_option;
    options.split(';', option_list);

    bool bBind = false;
    bool do_template = false;
    bool use_normals = true;

    int num_options = static_cast<int>(option_list.length());
    for (int i = 0; i < num_options; i++)
    {
        the_option.clear();
        option_list[i].split('=', the_option);
        if (the_option.length() < 1)
        {
            continue;
        }

        if (the_option[0] == "importSkl" && the_option.length() > 1)
        {
            bBind = (the_option[1].asUnsigned() != 0);
        } 
        else if (the_option[0] == "template" && the_option.length() > 1)
        {
            do_template = (the_option[1].asUnsigned() != 0);
        }
        else if (the_option[0] == "normals" && the_option.length() > 1)
        {
            use_normals = (the_option[1].asUnsigned() != 0);
        }
    }

    MString file_base_name = file.name();
    int rindex = file_base_name.rindexW('.');
    if (rindex != -1)
        file_base_name = file_base_name.substringW(0, rindex - 1);

    MString file_full_base_name = file.fullName();
    rindex = file_full_base_name.rindexW('.');
    if (rindex != -1)
        file_full_base_name = file_full_base_name.substringW(0, rindex - 1);
    const MString skl_file_name = file_full_base_name + ".skl";

    ifstream fin_skn(skn_file_name.asChar(), ios::binary);
    if (!fin_skn)
        FAILURE("SknImporter: " + skn_file_name + " : could not be opened for reading");

    SknReader *skn_reader = new SknReader();

    SklReader *skl_reader = NULL;
    SklData *skl_data = NULL;
    ifstream fin_skl;

    if (bBind)
    {
        fin_skl.open(skl_file_name.asChar(), ios::binary);
        if (!fin_skl)
            FAILURE("SknImporter: " + skl_file_name + " : could not be opened for reading");

        skl_reader = new SklReader();
        skl_data = &(skl_reader->data_);

        if (MStatus::kFailure == skl_reader->read(fin_skl))
        {
            delete skn_reader;
            delete skl_reader;
            FAILURE("SknImporter: skl_reader->read(" + skl_file_name + "); failed");
        }
        if (MStatus::kFailure == skl_reader->loadData())
        {
            delete skn_reader;
            delete skl_reader;
            FAILURE("SknImporter: skl_reader->loadData(); failed");
        }
    }
    if (MStatus::kFailure == skn_reader->read(fin_skn))
    {
        delete skn_reader;
        if (skl_reader)
            delete skl_reader;
        FAILURE("SknImporter: skn_reader->read(" + skn_file_name + "); failed");
    }
    if (MStatus::kFailure == skn_reader->loadData(file_base_name, use_normals, skl_data))
    {
        delete skn_reader;
        if (skl_reader)
            delete skl_reader;
        FAILURE("SknImporter: skn_reader->loadData(); failed");
    }
    if (do_template)
    {
        if (MStatus::kFailure == skl_reader->templateUnused())
        {
            delete skn_reader;
            if (skl_reader)
                delete skl_reader;
            FAILURE("SknImporter: skl_reader->templateUnused(); failed");
        }
    }

    fin_skn.close();
    delete skn_reader;
    if (skl_reader)
    {
        fin_skl.close();
        delete skl_reader;
    }

    MGlobal::displayInfo("SknImporter: import from " + skn_file_name + " successful!");
    return MS::kSuccess;
}

MPxFileTranslator::MFileKind SknImporter::identifyFile(
    const MFileObject& /*file*/, 
    const char* buffer, 
    short size) const
{
    int magic = 0x00112233;

    if (size >= 8 && !strncmp(buffer, reinterpret_cast<char*>(&magic), 4))
    {
        return kIsMyFileType;
    }

    return    kNotMyFileType;
}

} // namespace riot

