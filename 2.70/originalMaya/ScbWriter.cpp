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

#include <ScbWriter.h>

#include <maya/MGlobal.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MIntArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFloatPointArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>
#include <maya/MBoundingBox.h>

#include <maya_misc.h>

namespace riot {

MStatus ScbWriter::write(ostream& file)
{
    data_.switchHand();

    // magic
    char magic[9] = "r3d2Mesh";
    file.write(magic, 8);

    // set version
    int version = 0x20002;
    file.write(reinterpret_cast<char*>(&version), 4);

    // set name
    file.write(data_.name, ScbData::kNameLen);

    // set nums
    int num_vtx = data_.num_vtxs - 1;
    file.write(reinterpret_cast<char*>(&num_vtx), 4);
    num_vtx++;
    int num_faces = data_.num_indices / 3;
    file.write(reinterpret_cast<char*>(&num_faces), 4);

    // set is_colored;
    int colored = 0;
    file.write(reinterpret_cast<char*>(&colored), 4);

    // set transform
    file.write(reinterpret_cast<char*>(&data_.bbx), 4);
    file.write(reinterpret_cast<char*>(&data_.bby), 4);
    file.write(reinterpret_cast<char*>(&data_.bbz), 4);
    file.write(reinterpret_cast<char*>(&data_.bbdx), 4);
    file.write(reinterpret_cast<char*>(&data_.bbdy), 4);
    file.write(reinterpret_cast<char*>(&data_.bbdz), 4);

    // set vertices
    for (int i = 0; i < num_vtx; i++)
    {
        ScbVtx vtx = data_.vertices[i];
        file.write(reinterpret_cast<char*>(&vtx), ScbVtx::kSizeInFile);
    }
    
    // set faces
    for (int i = 0; i < num_faces; i++)
    {
        int offset = (i * 3);
        // set indices
        for (int k = 0; k < 3; k++)
        {
            int index = data_.indices[offset + k];
            file.write(reinterpret_cast<char*>(&index), 4);
        }

        // set mat_name[64];
        ScbMaterial mat = data_.materials.at(data_.shader_per_triangle[i]);
        file.write(mat.name, ScbMaterial::kNameLen);

        // set u_vec
        for (int k = 0; k < 3; k++)
        {
            float U = static_cast<float>(data_.u_vec[offset + k]);
            file.write(reinterpret_cast<char*>(&U), 4);
        }

        // set v_vec
        for (int k = 0; k < 3; k++)
        {
            float V = static_cast<float>(data_.v_vec[offset + k]);
            file.write(reinterpret_cast<char*>(&V), 4);
        }
    }

    /*
    if (is_colored)
    {
        int num_indices = num_faces * 3;
        for (int i = 0; i < num_indices; i++)
        {
            char color_component[3];
            file.write(color_component, 3);
            MColor color(color_component[0], color_component[1], color_component[2]); 
            data_.colors.push_back(color);
        }
    }
    */

    return MS::kSuccess;
}

MStatus ScbWriter::dumpData()
{
    MStatus status;
    MDagPath dag_path;
    MDagPath mesh_dag_path;
    MFnSkinCluster fn_skin_cluster;

    MSelectionList selection_list;
    if (MStatus::kSuccess != MGlobal::getActiveSelectionList(selection_list))
        FAILURE("ScbWriter: MGlobal::getActiveSelectionList()");

    MItSelectionList it_selection_list(selection_list, MFn::kMesh, &status);    
    if (status != MStatus::kSuccess)
        FAILURE("ScbWriter: it_selection_list()");

    if (it_selection_list.isDone())
        FAILURE("ScbWriter: no mesh selected!");

    it_selection_list.getDagPath(mesh_dag_path);
    it_selection_list.next();

    if (!it_selection_list.isDone())
        FAILURE("ScbWriter: more than one mesh selected!");

    MFnMesh mesh(mesh_dag_path);

    strcpy_s(data_.name, ScbData::kNameLen, mesh.name().asChar());

    // get shaders
    int instance_num = 0;
    if (mesh_dag_path.isInstanced())
        instance_num = mesh_dag_path.instanceNumber();

    MObjectArray shaders;
    MIntArray shader_indices;
    mesh.getConnectedShaders(instance_num, shaders, shader_indices);
    int shader_count = shaders.length();

    MGlobal::displayInfo(MString("shaders for this mesh : ") + shader_count);

    // check for holes
    MIntArray hole_info_array;
    MIntArray hole_vertex_array;
    mesh.getHoles(hole_info_array, hole_vertex_array);
    if (hole_info_array.length() != 0)
        FAILURE("ScbWriter: mesh contains holes");

    // check for triangulation
    MItMeshPolygon mesh_polygon_iter(mesh_dag_path);
    for (mesh_polygon_iter.reset(); !mesh_polygon_iter.isDone(); mesh_polygon_iter.next())
    {
        if (!mesh_polygon_iter.hasValidTriangulation())
            FAILURE("ScbWriter: a poly has no valid triangulation");
    }

    // get transform
    MFnTransform transform_node(mesh.parent(0));
    MVector pos = transform_node.getTranslation(MSpace::kTransform);

    // get vertices
    int num_vertices = mesh.numVertices();
    data_.num_vtxs = num_vertices;
    MFloatPointArray vertex_array;
    mesh.getPoints(vertex_array, MSpace::kWorld); // kObject is done by removing translation values
                                                 // that way scales and rotations are kept.

    for (int i = 0; i < num_vertices; i++)
    {
        ScbVtx vtx;
        vtx.x = vertex_array[i].x - static_cast<float>(pos.x);
        vtx.y = vertex_array[i].y - static_cast<float>(pos.y);
        vtx.z = vertex_array[i].z - static_cast<float>(pos.z);

        // check for min
        if (vtx.x < data_.bbx || i == 0)
            data_.bbx = vtx.x;
        if (vtx.y < data_.bby || i == 0)
            data_.bby = vtx.y;
        if (vtx.z < data_.bbz || i == 0)
            data_.bbz = vtx.z;
        // check for max
        if (vtx.x > data_.bbdx || i == 0)
            data_.bbdx = vtx.x;
        if (vtx.y > data_.bbdy || i == 0)
            data_.bbdy = vtx.y;
        if (vtx.z > data_.bbdz || i == 0)
            data_.bbdz = vtx.z;

        data_.vertices.push_back(vtx);
    }

    // bbd is the size so :
    data_.bbdx -= data_.bbx;
    data_.bbdy -= data_.bby;
    data_.bbdz -= data_.bbz;

    // get UVs
    MFloatArray u_array;
    MFloatArray v_array;
    mesh.getUVs(u_array, v_array);
    MIntArray uv_counts;
    MIntArray uv_ids;
    mesh.getAssignedUVs(uv_counts, uv_ids);

    // get triangles
    MIntArray triangle_counts;
    MIntArray triangle_vertices;
    mesh.getTriangles(triangle_counts, triangle_vertices);
    int numPolys = mesh.numPolygons();

    // fill data
    int cur = 0;
    for (int i = 0; i < numPolys; i++)
    {
        int triangle_count = triangle_counts[i];
        int shader = shader_indices[i];
        for (int j = 0; j < triangle_count; j++)
            data_.shader_per_triangle.push_back(shader);
        for (int j = 0; j < triangle_count * 3; j++)
        {
            
            data_.indices.push_back(triangle_vertices[cur]);
            data_.u_vec.push_back(u_array[uv_ids[cur]]);
            data_.v_vec.push_back(1 - v_array[uv_ids[cur]]);
            cur++;
        }
    }
    data_.num_indices = cur;

    // fill materials
    for (int i = 0; i < shader_count; i++)
    {
        ScbMaterial material;
        
        // get the plug for SurfaceShader
        MPlug shader_plug = MFnDependencyNode(shaders[i]).findPlug("surfaceShader");

        // get the connections to this plug
        MPlugArray plug_array;
        shader_plug.connectedTo(plug_array, true, false, &status);

        // first connection is material.
        MFnDependencyNode surface_shader(plug_array[0].node());

        strcpy_s(material.name, ScbMaterial::kNameLen, surface_shader.name().asChar());

        data_.materials.push_back(material);
    }

    return MS::kSuccess;
}

} // namespace riot

