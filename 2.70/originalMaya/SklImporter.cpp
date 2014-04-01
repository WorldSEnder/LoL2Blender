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

#include <SklImporter.h>

#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MIOStream.h>
#include <maya/MFStream.h>

#include <maya_misc.h>

#include <SklReader.h>

namespace riot {

void* SklImporter::creator()
{
    return new SklImporter();
}

MStatus SklImporter::reader(const MFileObject& file, 
                         const MString& options, 
                         MPxFileTranslator::FileAccessMode mode) 
{
    if (MPxFileTranslator::kImportAccessMode != mode)
    {
        MGlobal::displayInfo("SklImporter: only support \"import\"");
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

    MStringArray option_list;
    MStringArray the_option;
    options.split(';', option_list);

    bool do_template = false;

    int num_options = static_cast<int>(option_list.length());
    for (int i = 0; i < num_options; i++)
    {
        the_option.clear();
        option_list[i].split('=', the_option);
        if (the_option.length() < 1)
            continue;

        if (the_option[0] == "template" && the_option.length() > 1)
        {
            do_template = (the_option[1].asUnsigned() != 0);
        }
    }

    ifstream fin(file_name.asChar(), ios::binary);

    if (!fin)
        FAILURE("SklImporter: " + file_name + " : could not be opened for reading");

    SklReader *reader = new SklReader();

    if (MStatus::kFailure == reader->read(fin))
    {
        delete reader;
        FAILURE("SklImporter: reader->read(filename); failed");
    }

    if (MStatus::kFailure == reader->loadData())
    {
        delete reader;
        FAILURE("SklImporter: reader->loadData(): failed");
    }

    if (do_template)
    {
        if (MStatus::kFailure == reader->templateUnused())
        {
            delete reader;
            FAILURE("SklImporter: reader->templateUnused(): failed");
        }
    }

    //fin.flush();
    fin.close();
    delete reader;

    MGlobal::displayInfo("SklImporter: import from " + file_name + " successful!");
    return MS::kSuccess;
}

MPxFileTranslator::MFileKind SklImporter::identifyFile(
    const MFileObject& /*file*/, 
    const char* buffer, 
    short size) const
{
    if (size >= 20 &&
        !strncmp(buffer, "r3d2sklt", 8) &&
        (buffer[8] == 2 || buffer[8] == 1))
    {
        return kIsMyFileType;
    }

    return    kNotMyFileType;
}

} // namespace riot

