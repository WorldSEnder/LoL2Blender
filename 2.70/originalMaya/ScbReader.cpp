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

#include <ScbReader.h>

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
#include <maya/MColorArray.h>

#include <maya_misc.h>

namespace riot {

MStatus ScbReader::read(istream& file)
{
    // get length
    int minlen = 152;
    file.seekg (0, ios::end);
    int length = file.tellg();
    file.seekg (0, ios::beg);

    // check minimum length
    if (length < minlen)
        FAILURE("ScbReader: the file is empty!");

    // check magic
    char magic[8];
    file.read(magic, 8);
    if (strncmp(magic, "r3d2Mesh", 8))
        FAILURE("ScbReader: magic is wrong!");

    // get version
    int version;
    file.read(reinterpret_cast<char*>(&version), 4);
    if (version != 0x20002 && version != 0x20001)
        FAILURE("ScbReader: anm type not supported, \n please report that to ThiSpawn");
    data_.version = version;

    // get name
    file.read(data_.name, ScbData::kNameLen);

    // get nums
    int num_vtx;
    file.read(reinterpret_cast<char*>(&num_vtx), 4);
    num_vtx++;
    data_.num_vtxs = num_vtx;
    int num_faces;
    file.read(reinterpret_cast<char*>(&num_faces), 4);
    data_.num_indices = num_faces * 3;

    // get is_colored;
    int colored;
    file.read(reinterpret_cast<char*>(&colored), 4);
    bool is_colored = (colored != 0);
    data_.is_colored  = is_colored;

    // get transform
    if (version == 0x20002)
    {
        file.read(reinterpret_cast<char*>(&data_.bbx), 4);
        file.read(reinterpret_cast<char*>(&data_.bby), 4);
        file.read(reinterpret_cast<char*>(&data_.bbz), 4);
        file.read(reinterpret_cast<char*>(&data_.bbdx), 4);
        file.read(reinterpret_cast<char*>(&data_.bbdy), 4);
        file.read(reinterpret_cast<char*>(&data_.bbdz), 4);
    }

    // get vertices
    for (int i = 0; i < num_vtx; i++)
    {
        ScbVtx vtx;
        file.read(reinterpret_cast<char*>(&vtx), ScbVtx::kSizeInFile);
        data_.vertices.push_back(vtx);
    }
    
    // get faces
    char mat_name[ScbMaterial::kNameLen];
    bool did_face_fail;
    for (int i = 0; i < num_faces; i++)
    {
        did_face_fail = false;
        // get indices
        int indices[3];
        file.read(reinterpret_cast<char*>(&indices), 12);
        // check if that can build a triangle
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
            MGlobal::displayWarning("ScbReader: input mesh has a badly built triangle, removing it...");
            data_.num_indices -= 3;
            did_face_fail = true;
        }
        else
        {
            data_.indices.push_back(indices[0]);
            data_.indices.push_back(indices[1]);
            data_.indices.push_back(indices[2]);
        }

        // get mat_name[64];
        file.read(mat_name, ScbMaterial::kNameLen);
        if (!did_face_fail)
        {
            int c = 0;
            for (; c < ScbMaterial::kNameLen; c++)
            {
                if (mat_name[c] == '\0')
                    break;
            }
            if (c == ScbMaterial::kNameLen)
                MGlobal::displayWarning("ScbReader: material name too long\nreport this error to ThiSpawn");

            int j = 0;
            int data_materials_size = static_cast<int>(data_.materials.size());
            for (; j < data_materials_size; j++)
            {
                if (!strcmp(data_.materials.at(j).name, mat_name))
                    break;
            }
            if (j == data_materials_size)
            {
                MGlobal::displayInfo(MString("found new material : ") + mat_name);
                ScbMaterial new_mat;
                strcpy_s(new_mat.name, ScbMaterial::kNameLen, mat_name);
                data_.materials.push_back(new_mat);
            }
            data_.shader_per_triangle.push_back(j);
        }

        // get u_vec
        for (int k = 0; k < 3; k++)
        {
            float u;
            file.read(reinterpret_cast<char*>(&u), 4);
            if (!did_face_fail)
                data_.u_vec.push_back(u);
        }

        // get v_vec
        for (int k = 0; k < 3; k++)
        {
            float v;
            file.read(reinterpret_cast<char*>(&v), 4);
            if (!did_face_fail)
                data_.v_vec.push_back(v);
        }
    }

    if (is_colored)
    {
        int num_indices = num_faces * 3;
        for (int i = 0; i < num_indices; i++)
        {
            char color_component[3];
            file.read(color_component, 3);
            MColor color(color_component[0], color_component[1], color_component[2]); 
            data_.colors.push_back(color);
        }
    }

    data_.switchHand();

    return MS::kSuccess;
}

MStatus ScbReader::loadData()
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
        ScbVtx vtx = data_.vertices[i];
        vertex_array.append(vtx.x, vtx.y, vtx.z);
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
        FAILURE("ScbReader: mesh.create() failed");

    // set uvs
    status = mesh.assignUVs(poly_counts, uv_ids);
    if (status != MS::kSuccess)
        FAILURE("ScbReader: mesh.assignUVs() failed");

    // set colors
    // always black so ... disabled
    /*
    if (data_.is_colored)
    {
        MColorArray colors;
        MIntArray faceList(num_indices);
        MIntArray vertexList(num_indices);
        for (int i = 0; i < num_triangles; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                int offset = (i * 3) + j;
                colors.append(data_.colors[offset]);
                faceList[offset] = i;
                vertexList[offset] = data_.indices[offset];
            }
        }
        mesh.setFaceVertexColors(colors, faceList, vertexList);
    }
    */

    // set names
    mesh.setName(data_.name);
    MFnTransform transform_node(mesh.parent(0));
    transform_node.setName(MString("transform") + data_.name);

    // get render partition
    MItDependencyNodes it_dep_node(MFn::kPartition, &status);
    if (status != MS::kSuccess)
        FAILURE("ScbReader: fn_dep_node.create() failed");
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
        ScbMaterial material = data_.materials[i];
        MObject shader = fn_lambert_shader.create();

        MString mat_name(material.name);
        fn_lambert_shader.setName(mat_name);

        // give it a color to make materials distinguishable
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
            FAILURE("ScbReader: fn_dep_node.create(\"shadingEngine\", ...) failed");

        // create material info
        MObject material_info = fn_dep_node.create("materialInfo", mat_name + "_material_info", &status);
        if (status != MS::kSuccess)
            FAILURE("ScbReader: fn_dep_node.create(\"materialInfo\", ...) failed");

        if (!found_partition)
        {
            MGlobal::displayWarning("ScbReader: renderPartition not found. " \
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
    
    // test to check if the 6 floats are about the bounding box
    // YES ! ^^
    /*
    MFnIkJoint fn_joint;
    fn_joint.create();
    MVector piv(data_.bbx, data_.bby, data_.bbz);
    fn_joint.setTranslation(piv, MSpace::kTransform);
    fn_joint.create();
    piv = MVector(data_.bbx + data_.bbdx, data_.bby + data_.bbdy, data_.bbz + data_.bbdz);
    fn_joint.setTranslation(piv, MSpace::kTransform);
    */

    mesh.updateSurface();
    return MS::kSuccess;
}

} // namespace riot

