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

#include <SknReader.h>

#include <maya/MGlobal.h>
#include <maya/MFnMesh.h>
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

#include <maya_misc.h>

namespace riot {

MStatus SknReader::read(istream& file)
{
    // get length
    int minlen = 8;
    file.seekg (0, ios::end);
    int length = file.tellg();
    file.seekg (0, ios::beg);

    // check minimum length
    if (length < minlen)
        FAILURE("SknReader: the file is empty!");

    // check magic
    int magic;
    file.read(reinterpret_cast<char*>(&magic), 4);
    if (magic != 0x00112233)
        FAILURE("SknReader: magic is wrong!");

    // get version
    USHORT version;
    file.read(reinterpret_cast<char*>(&version), 2);
    if (version > 2)
        FAILURE("SknReader: skn type not supported, \n please report that to ThiSpawn");
    data_.version = version;

    // get num obj
    USHORT num_objects;
    file.read(reinterpret_cast<char*>(&num_objects), 2);
    if (num_objects != 1)
        FAILURE("SknReader: more than 1 or no objects in the file.");

    // get materials
    if (version == 1 || version == 2)
    {
        minlen += 4;
        if (length < minlen)
            FAILURE("SknReader: unexpected end of file");

        int num_materials;
        file.read(reinterpret_cast<char*>(&num_materials), 4);

        minlen += SknMaterial::kSizeInFile * num_materials;
        if (length < minlen)
            FAILURE("SknReader: unexpected end of file");

        for (int i = 0; i < num_materials; i++)
        {
            SknMaterial material;
            file.read(reinterpret_cast<char*>(&material), SknMaterial::kSizeInFile);
            data_.materials.push_back(material);
        }
    }

    // check minimum length
    minlen += 8;
    if (length < minlen)
        FAILURE("SknReader: unexpected end of file");

    // get nums
    int num_indices;
    file.read(reinterpret_cast<char*>(&num_indices), 4);
    data_.num_indices = num_indices;
    int num_vertices;
    file.read(reinterpret_cast<char*>(&num_vertices), 4);
    data_.num_vtxs = num_vertices;
    data_.num_final_vtxs = num_vertices;

    if (num_indices % 3 != 0)
        FAILURE("SknReader: num_indices % 3 != 0 ...");

    // check minimum length
    minlen += 2 * num_indices + SknVtx::kSizeInFile * num_vertices;
    if (length < minlen)
        FAILURE("SknReader: unexpected end of file");
    

    // get indices
    int num_triangles = num_indices / 3;
    for (int i = 0; i < num_triangles; i++)
    {
        USHORT indices[3];
        file.read(reinterpret_cast<char*>(&indices), 6);
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
            MGlobal::displayWarning("SknReader: input mesh has a badly built triangle, removing it...");
            data_.num_indices -= 3; 
        }
        else
        {
            data_.indices.push_back(indices[0]);
            data_.indices.push_back(indices[1]);
            data_.indices.push_back(indices[2]);
        }
    }

    // get vertices
    for (int i = 0; i < num_vertices; i++)
    {
        SknVtx vtx;
        file.read(reinterpret_cast<char*>(&vtx), SknVtx::kSizeInFile);
        data_.vertices.push_back(vtx);
    }

    // get endtab
    if (version == 2)
        file.read(reinterpret_cast<char*>(&(data_.endTab)), 12);

    data_.switchHand();

    return MS::kSuccess;
}

MStatus SknReader::loadData(MString& name, bool use_normals, SklData* skl_data)
{
    MFnSkinCluster fn_skin_cluster;
    MFnLambertShader fn_lambert_shader;
    MFnDependencyNode fn_dep_node;
    MDGModifier dg_modifier;
    MFnSet fn_set;
    MStatus status;
    MFnMesh mesh;
    MFloatPointArray vertex_array;
    int num_triangles = data_.num_indices / 3;
    MIntArray poly_counts(num_triangles);
    MIntArray poly_connects(data_.num_indices);
    MFloatArray u_array(data_.num_vtxs);
    MFloatArray v_array(data_.num_vtxs);
    MVectorArray normals(data_.num_vtxs);
    MIntArray normals_indices(data_.num_vtxs);
    MDagPath mesh_dag_path;

    // set indices data
    for (int i = 0; i < num_triangles; i++)
        poly_counts[i] = 3;

    for (int i = 0; i < data_.num_indices; i++)
        poly_connects[i] = data_.indices[i];

    // set vertices data
    for (int i = 0; i < data_.num_vtxs; i++)
    {
        SknVtx vtx = data_.vertices[i];
        u_array[i] = vtx.U;
        v_array[i] = 1 - vtx.V;
        if (u_array[i] > 1)
            MGlobal::displayWarning(MString("SknReader: U out of bound (>1): ") + i);
        if (u_array[i] < 0)
            MGlobal::displayWarning(MString("SknReader: U out of bound (<0): ") + i);
        if (v_array[i] > 1)
            MGlobal::displayWarning(MString("SknReader: V out of bound (>1): ") + i);
        if (v_array[i] < 0)
            MGlobal::displayWarning(MString("SknReader: V out of bound (<0): ") + i);
        vertex_array.append(vtx.x, vtx.y, vtx.z);
        normals[i] = MVector(vtx.normal[0], vtx.normal[1], vtx.normal[2]);
        normals_indices[i] = static_cast<int>(i);
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
        FAILURE("SknReader: mesh.create() failed");

    if (use_normals)
    {
        if (data_.num_indices == data_.num_vtxs)
        {
            MGlobal::displayWarning("SknReader: exception, normals not locked cause no shared normals");
        }
        else
        {
            //status = mesh.lockVertexNormals(normals_indices);
            //if (status != MS::kSuccess)
            //    FAILURE("SknReader: mesh.lockVertexNormals() failed");
            //MGlobal::displayInfo("SknReader: normals locked");
            // set normals (after the lock)
            status = mesh.setVertexNormals(normals, normals_indices);
            if (status != MS::kSuccess)
                FAILURE("SknReader: mesh.setVertexNormals() failed");
        }
    }

    // set uvs
    status = mesh.assignUVs(poly_counts, poly_connects);
    if (status != MS::kSuccess)
        FAILURE("SknReader: mesh.assignUVs() failed (" + status.errorString() + ")");

    // set names
    mesh.setName(name);
    MFnDependencyNode transform_node(mesh.parent(0));
    transform_node.setName("transform" + name);

    // get render partition
    MItDependencyNodes it_dep_node(MFn::kPartition, &status);
    if (status != MS::kSuccess)
        FAILURE("SknReader: fn_dep_node.create() failed");
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
    if (data_.version > 0)
    {
        int data_materials_size = static_cast<int>(data_.materials.size());
        for (int i = 0; i < data_materials_size; i++)
        {
            SknMaterial material = data_.materials[i];
            MObject shader = fn_lambert_shader.create();

            MString mat_name(material.name);
            fn_lambert_shader.setName(mat_name);
            MColor color;
            if (i == 1)
            {
                color.set(MColor::kRGB, 0.180f, 0.740f, 0.780f);
                fn_lambert_shader.setColor(color);
            }

            // create shading engine
            MObject shading_engine = fn_dep_node.create("shadingEngine", mat_name + "_SG", &status);
            if (status != MS::kSuccess)
                FAILURE("SknReader: fn_dep_node.create() failed");

            // create material info
            MObject material_info = fn_dep_node.create("materialInfo", mat_name + "_material_info", &status);
            if (status != MS::kSuccess)
                FAILURE("SknReader: fn_dep_node.create() failed");

            if (!found_partition)
            {
                MGlobal::displayWarning("SknReader: renderPartition not found. " \
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
            int endIndex = (material.startIndex + material.num_indices) / 3;
            for (int j = material.startIndex / 3; j < endIndex; j++)
                group_poly_indices.append(j);
            fn_comp.addElements(group_poly_indices);

            fn_set.setObject(shading_engine);
            fn_set.addMember(mesh_dag_path, face_comp);
        }
    }
    else
    {
        MObject shader = fn_lambert_shader.create();

        // create shading engine
        MObject shading_engine = fn_dep_node.create("shadingEngine", name + "_SG", &status);
        if (status != MS::kSuccess)
            FAILURE("SknReader: fn_dep_node.create() failed");

        // create material info
        MObject material_info = fn_dep_node.create("materialInfo", name + "_material_info", &status);
        if (status != MS::kSuccess)
            FAILURE("SknReader: fn_dep_node.create() failed");

        if (!found_partition)
            MGlobal::displayWarning("SknReader: renderPartition not found.  SG not added to the SG list.");
        else
        {
            fn_render_partition.addMember(shading_engine);
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

        fn_set.setObject(shading_engine);
        fn_set.addMember(mesh.object());
    }

    if (!skl_data)
    {
        mesh.updateSurface();    // don't do it before bind.
                                // to prevent vertex ids from changing.
        return MS::kSuccess;
    }
    
    skl_data->joints;

    MSelectionList selectList;
    selectList.add(mesh_dag_path);
    for (int i = 0; i < skl_data->num_indices; i++)
    {
        selectList.add(skl_data->joints[skl_data->skn_indices[i]]);
    }
    MGlobal::selectCommand(selectList);
    MGlobal::executeCommand("skinCluster -mi 4 -tsb -n skinCluster_" + name);

    MPlug in_mesh_plug = mesh.findPlug("inMesh");
    MPlugArray in_mesh_connections;
    in_mesh_plug.connectedTo(in_mesh_connections, true, false);
    if (in_mesh_connections.length() == 0)
        FAILURE("SknReader: failed to find the created skinCluster.");
    MPlug output_geom_plug = in_mesh_connections[0];

    fn_skin_cluster.setObject(output_geom_plug.node());
    MDagPathArray influences_dag_path;
    fn_skin_cluster.influenceObjects(influences_dag_path);

    // get influence indices by skn index
    // using this array for setweights allow
    // to use skn index as value's id per component.
    // note : the selectList i gave makes the skincluster using
    // the sknid order for physical indices
    MIntArray influenceIndicesBySknId(skl_data->num_indices);
    if (static_cast<int>(influences_dag_path.length()) != skl_data->num_indices)
        FAILURE("SknReader: influences_dag_path.length() != skl_data->num_indices");

    int num_influences = skl_data->num_indices;
    for (int i = 0; i < num_influences; i++)
    {
        MDagPath jointPath = skl_data->joints[skl_data->skn_indices[i]];
        int j = 0;
        for (; j < num_influences; j++)
        {
            if (influences_dag_path[j] == jointPath)
                break;
        }
        if (j == static_cast<int>(influences_dag_path.length()))
            FAILURE("SknReader: unable to find a bound bone.\
                        this error should not happen!");
        
        influenceIndicesBySknId[i] = j;
    }

    MFnSingleIndexedComponent fn_comp;
    MObject vtx_comp = fn_comp.create(MFn::kMeshVertComponent);
    
    MIntArray group_vtx_indices(data_.num_vtxs);
    // create values per vertex (component)
    // and add the vertex to the components.
    for (int i = 0; i < data_.num_vtxs; i++)
        group_vtx_indices[i] = i;
    fn_comp.addElements(group_vtx_indices);

    MString skinClusterName = fn_skin_cluster.name();
    MGlobal::executeCommand("setAttr " + skinClusterName + ".normalizeWeights 0");
    MGlobal::displayInfo("SknReader: setting weights ...");

    MDoubleArray values(data_.num_vtxs * num_influences);

    for (int i = 0; i < data_.num_vtxs; i++)
    {
        SknVtx vtx = data_.vertices[i];

        for (int j = 0; j < 4; j++)
        {
            double weight = vtx.weights[j];
            
            int n = vtx.skn_indices[j];
            if (weight != 0)
                values[i * num_influences + n] = weight;

            if (n >= num_influences)
                FAILURE(MString("SknReader: sknId number ") + j + "is out of range for vtx[" + i + "]");
        }
    }


    status = fn_skin_cluster.setWeights(mesh_dag_path, vtx_comp, influenceIndicesBySknId, values, false); 
    if (status != MS::kSuccess)
        FAILURE(status.errorString());

    MGlobal::executeCommand("setAttr " + skinClusterName + ".normalizeWeights 1");
    MGlobal::executeCommand("skinPercent -normalize true " + fn_skin_cluster.name() + " " + mesh.name());

    mesh.updateSurface();
    return MS::kSuccess;
}

} // namespace riot

