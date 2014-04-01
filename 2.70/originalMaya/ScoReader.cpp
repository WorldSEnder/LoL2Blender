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

#include <ScoReader.h>

#include <maya/MGlobal.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MIntArray.h>
#include <maya/MVectorArray.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnSet.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MDGModifier.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MFnPartition.h>
#include <maya/MSelectionList.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MDagPath.h>
#include <maya/MFnIkJoint.h>

#include <maya_misc.h>

namespace riot {

MStatus ScoReader::read(istream& file)
{
    char buffer[kMaxBufLen];
    char buffer1[kMaxBufLen];
    char attr_name[kMaxBufLen];

    // check magic
    file.getline(buffer, kMaxBufLen);
    if (_strnicmp(buffer, "[ObjectBegin]", 13))
        FAILURE("ScoReader: magic is wrong!");

    // get name
    file.getline(buffer, kMaxBufLen);
    sscanf_s(buffer, "%s %s", attr_name, kMaxBufLen, buffer1, kMaxBufLen);
    if (_stricmp("Name=", attr_name))
        FAILURE("ScoReader: Invalid SCO");

    data_.name = buffer1;

    // get central point
    file.getline(buffer, kMaxBufLen);
    sscanf_s(buffer, "%s %f %f %f", attr_name, kMaxBufLen, &data_.tx, &data_.ty, &data_.tz);

    //MGlobal::displayInfo(MString("CentralPoint= ") + data_.tx + " " + data_.ty + " " + data_.tz);

    // get pivot point
    file.getline(buffer, kMaxBufLen);
    sscanf_s(buffer, "%s", attr_name, kMaxBufLen);
    if (!strncmp(attr_name, "PivotPoint=", 11))
    {
        sscanf_s(buffer, "%s %f %f %f", attr_name, kMaxBufLen, &data_.px, &data_.py, &data_.pz);
        data_.use_pivot = true;
        file.getline(buffer, kMaxBufLen);
    }

    // get vertices
    int num_vtx; // but red as int
    sscanf_s(buffer, "%s %d", attr_name, kMaxBufLen, &num_vtx);
    data_.num_vtxs = num_vtx;
    for (int i = 0; i < num_vtx; i++)
    {
        ScoVtx vtx;
        file.getline(buffer, kMaxBufLen);
        sscanf_s(buffer, "%f %f %f", &vtx.x, &vtx.y, &vtx.z);
        data_.vertices.push_back(vtx);
    }

    // get faces
    int num_faces; // but red as int
    file.getline(buffer, kMaxBufLen);
    sscanf_s(buffer, "%s %d", attr_name, kMaxBufLen, &num_faces);
    data_.num_indices = num_faces * 3;
    for (int i = 0; i < num_faces; i++)
    {
        char *pch;
        char *next_token;
        MString mat_name;
        int indices[3];
        file.getline(buffer, kMaxBufLen);
        pch = strtok_s(buffer, " \t", &next_token);
        int vertexCount = atoi(pch);
        if (vertexCount != 3)
            FAILURE("ScoReader: vertexCount for a face is != 3");
        pch = strtok_s(0, " \t", &next_token);
        indices[0] = atoi(pch);
        pch = strtok_s(0, " \t", &next_token);
        indices[1] = atoi(pch);
        pch = strtok_s(0, " \t", &next_token);
        indices[2] = atoi(pch);
        pch = strtok_s(0, " \t", &next_token);
        mat_name = pch;
        pch += mat_name.length() + 1;

        if (indices[0] == indices[1] ||
            indices[0] == indices[2] ||
            indices[1] == indices[2] ||
            indices[0] < 0 ||
            indices[0] >= data_.num_vtxs ||
            indices[1] < 0 ||
            indices[1] >= data_.num_vtxs ||
            indices[2] < 0 ||
            indices[2] >= data_.num_vtxs)
        {
            MGlobal::displayWarning("ScoReader: input mesh has a badly built triangle, removing it...");
            data_.num_indices -= 3;
        }
        else
        {
            int j = 0;
            int data_materials_size = static_cast<int>(data_.materials.size());
            for (; j < data_materials_size; j++)
            {
                if (data_.materials.at(j).name == mat_name)
                    break;
            }
            if (j == data_materials_size)
            {
                MGlobal::displayInfo("found new material : " + mat_name);
                ScoMaterial new_mat;
                new_mat.name = mat_name;
                if (mat_name.length() > ScoMaterial::kMaxNameLen)
                    MGlobal::displayWarning("ScoReader: material name too long\nreport this error to ThiSpawn");
                data_.materials.push_back(new_mat);
            }
            
            data_.shader_per_triangle.push_back(j);

            for (int k = 0; k < 3; k++)
            {
                data_.indices.push_back(indices[k]);
                data_.u_vec.push_back(strtod(pch, &pch));
                data_.v_vec.push_back(strtod(pch, &pch));
            }
        }
    }

    data_.switchHand();

    return MS::kSuccess;
}

MStatus ScoReader::loadData()
{
    MFnSkinCluster fn_skin_cluster;
    MFnLambertShader fn_lambert_shader;
    MFnDependencyNode fn_dep_node;
    MDGModifier dg_modifier;
    MFnSet fn_set;
    MStatus status;
    MFnMesh mesh;
    MFloatPointArray vertex_array;
    int num_indices = data_.num_indices;
    int num_triangles = num_indices / 3;
    MIntArray poly_counts(num_triangles);
    MIntArray poly_connects(num_indices);
    MIntArray uv_ids(num_indices);
    MFloatArray u_array(num_indices);
    MFloatArray v_array(num_indices);
    MDagPath mesh_dag_path;

    // set indices data
    for (int i = 0; i < num_triangles; i++)
        poly_counts[i] = 3;

    // set uv_ids and poly_connects
    for (int i = 0; i < num_indices; i++)
    {
        poly_connects[i] = data_.indices[i];
        uv_ids[i] = i;
        u_array[i] = static_cast<float>(data_.u_vec[i]);
        v_array[i] = static_cast<float>(1 - data_.v_vec[i]);
    }

    // set vertices data
    for (int i = 0; i < data_.num_vtxs; i++)
    {
        ScoVtx vtx = data_.vertices[i];
        vertex_array.append(vtx.x - data_.tx, vtx.y - data_.ty, vtx.z - data_.tz);
    }

    //create mesh
    mesh.create(
        data_.num_vtxs, 
        num_triangles,
        vertex_array, 
        poly_counts, 
        poly_connects, 
        u_array, 
        v_array, 
        MObject::kNullObj, 
        &status
    );

    mesh.getPath(mesh_dag_path);

    if (status != MS::kSuccess)
        FAILURE("ScoReader: mesh.create() failed");

    // set uvs
    status = mesh.assignUVs(poly_counts, uv_ids);
    if (status != MS::kSuccess)
        FAILURE("ScoReader: mesh.assignUVs() failed");

    // set names
    mesh.setName(data_.name);
    MFnTransform transform_node(mesh.parent(0));
    transform_node.setName("transform" + data_.name);

    // set translation
    MVector vec(data_.tx, data_.ty, data_.tz);
    transform_node.setTranslation(vec, MSpace::kTransform);

    // get render partition
    MItDependencyNodes it_dep_node(MFn::kPartition, &status);
    if (status != MS::kSuccess)
        FAILURE("ScoReader: fn_dep_node.create() failed");
    MFnPartition fn_render_partition;
    bool found_partition = false;
    for (; !it_dep_node.isDone(); it_dep_node.next())
    {
        fn_render_partition.setObject(it_dep_node.thisNode());
        if (fn_render_partition.name() == "renderPartition" && fn_render_partition.isRenderPartition())
        {
            found_partition = true;
            break;
        }
    }

    // create and assign materials
    int data_materials_size = static_cast<int>(data_.materials.size());
    for (int i = 0; i < data_materials_size; i++)
    {
        ScoMaterial material = data_.materials[i];
        MObject shader = fn_lambert_shader.create();

        MString mat_name(material.name);
        fn_lambert_shader.setName(mat_name);
        MColor color;
        if (i == 1)
        {
            color.set(MColor::kRGB, 0.180f, 0.740f, 0.780f);
            fn_lambert_shader.setColor(color);
        }
        if (i == 2)
        {
            color.set(MColor::kRGB, 0.534f, 0.667f, 0.261f);
            fn_lambert_shader.setColor(color);
        }
        if (i == 3)
        {
            color.set(MColor::kRGB, 0.588f, 0.254f, 0.523f);
            fn_lambert_shader.setColor(color);
        }

        // create shading engine
        MObject shading_engine = fn_dep_node.create("shadingEngine", mat_name + "_SG", &status);
        if (status != MS::kSuccess)
            FAILURE("ScoReader: fn_dep_node.create() failed");

        // create material info
        MObject material_info = fn_dep_node.create("materialInfo", mat_name + "_material_info", &status);
        if (status != MS::kSuccess)
            FAILURE("ScoReader: fn_dep_node.create() failed");

        if (!found_partition)
        {
            MGlobal::displayWarning("ScoReader: renderPartition not found. " \
                                    + mat_name + "_SG not added to the SG list.");
        }
        else
        {
            MPlug partition_plug = MFnDependencyNode(shading_engine).findPlug("partition");
            MPlug sets_plug = fn_render_partition.findPlug("sets");
            sets_plug = riot::firstNotConnectedElement(sets_plug);
            dg_modifier.connect(partition_plug, sets_plug);
        }

        // connect shader.outColor -> shading_engine.surface_shader
        MPlug out_color_plug = fn_lambert_shader.findPlug("outColor");
        MPlug surface_shader_plug = MFnDependencyNode(shading_engine).findPlug("surfaceShader");
        dg_modifier.connect(out_color_plug, surface_shader_plug);
        // connect shading_engine.message -> material_info.shadingGroup
        MPlug message_plug = MFnDependencyNode(shading_engine).findPlug("message", &status);
        MPlug shading_group_plug = MFnDependencyNode(material_info).findPlug("shadingGroup", &status);
        dg_modifier.connect(message_plug, shading_group_plug);

        dg_modifier.doIt();

        MFnSingleIndexedComponent fn_comp;
        MObject face_comp = fn_comp.create(MFn::kMeshPolygonComponent);
        MIntArray group_poly_indices;
        for (int j = 0; j < num_triangles; j++)
        {
            if (data_.shader_per_triangle[j] == i)
                group_poly_indices.append(j);
        }
        fn_comp.addElements(group_poly_indices);

        fn_set.setObject(shading_engine);
        fn_set.addMember(mesh_dag_path, face_comp);
    }
    
    if (data_.use_pivot)
    {
        MFnIkJoint fn_joint;
        fn_joint.create();
        fn_joint.setName(MString("centralPivot_") + data_.name);
        MVector piv(data_.tx - data_.px, data_.ty - data_.py, data_.tz - data_.pz);
        fn_joint.setTranslation(piv, MSpace::kTransform);

        MDagPath jointPath;
        fn_joint.getPath(jointPath);

        MSelectionList selectList;
        selectList.add(mesh_dag_path);
        selectList.add(jointPath);

        MGlobal::selectCommand(selectList);
        MGlobal::executeCommand("skinCluster -mi 1 -tsb -n skinCluster_" + data_.name);
    }

    mesh.updateSurface();
    return MS::kSuccess;
}

} // namespace riot

