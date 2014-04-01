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

#include <ScoWriter.h>

#include <iomanip>

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

#include <maya_misc.h>

#include <ScoData.hpp>

namespace riot {

MStatus ScoWriter::write(ostream& file)
{
    data_.switchHand();

    // set magic
    file << "[ObjectBegin]" << std::endl;

    // set name
    if (data_.name.length() > ScoData::kMaxNameLen)
        data_.name = data_.name.substring(0, ScoData::kMaxNameLen - 1);
    file << "Name= " << data_.name << std::endl;

    file << std::fixed;
    file << std::setprecision(4);
    // set central point
    file << "CentralPoint= " << data_.tx << " " << data_.ty << " " << data_.tz << std::endl;

    // set pivot point
    if (data_.use_pivot)
        file << "PivotPoint= " << data_.px << " " << data_.py << " " << data_.pz << std::endl;
    
    // set vertices
    int num_vtx = data_.num_vtxs;
    file << "Verts= " << num_vtx << std::endl;
    for (int i = 0; i < num_vtx; i++)
    {
        ScoVtx vtx = data_.vertices.at(i);
        file << vtx.x << " " << vtx.y << " " << vtx.z << std::endl;
    }

    // set faces
    int num_faces = data_.num_indices / 3; // but red as int
    file << "Faces= " << num_faces << std::endl;

    for (int i = 0; i < num_faces; i++)
    {
        int offset = i * 3;
        file << 3 << '\t';
        file << std::setw(5) << data_.indices[offset];
        file << std::setw(5) << data_.indices[offset+1];
        file << std::setw(5) << data_.indices[offset+2];
        ScoMaterial mat = data_.materials.at(data_.shader_per_triangle[i]);
        if (mat.name.length() > ScoMaterial::kMaxNameLen)
            mat.name = mat.name.substring(0, ScoMaterial::kMaxNameLen - 1);
        file << '\t' << std::setw(20) << mat.name << '\t';
        file << std::setprecision(14);
        file << data_.u_vec[offset] << " ";
        file << data_.v_vec[offset] << " ";
        file << data_.u_vec[offset+1] << " ";
        file << data_.v_vec[offset+1] << " ";
        file << data_.u_vec[offset+2] << " ";
        file << data_.v_vec[offset+2];
        file << std::endl;
    }

    // set end
    file << "[ObjectEnd]" << std::endl;

    return MS::kSuccess;
}

MStatus ScoWriter::dumpData()
{
    MStatus status;
    MDagPath dag_path;
    MDagPath mesh_dag_path;
    MFnSkinCluster fn_skin_cluster;

    MSelectionList selection_list;
    if (MStatus::kSuccess != MGlobal::getActiveSelectionList(selection_list))
        FAILURE("ScoWriter: MGlobal::getActiveSelectionList()");

    MItSelectionList it_selection_list(selection_list, MFn::kMesh, &status);    
    if (status != MStatus::kSuccess)
        FAILURE("ScoWriter: it_selection_list()");

    if (it_selection_list.isDone())
        FAILURE("ScoWriter: no mesh selected!");

    it_selection_list.getDagPath(mesh_dag_path);
    it_selection_list.next();

    if (!it_selection_list.isDone())
        FAILURE("ScoWriter: more than one mesh selected!");

    MFnMesh mesh(mesh_dag_path);

    data_.name = mesh.name();

    MFnTransform transform_node(mesh.parent(0));
    MVector pos = transform_node.getTranslation(MSpace::kTransform);
    data_.tx = static_cast<float>(pos.x);
    data_.ty = static_cast<float>(pos.y);
    data_.tz = static_cast<float>(pos.z);

    // find skincluster
    MPlug in_mesh_plug = mesh.findPlug("inMesh");
    MPlugArray in_mesh_connections;
    in_mesh_plug.connectedTo(in_mesh_connections, true, false);
    if (in_mesh_connections.length() != 0)
    {
        MPlug output_geom_plug = in_mesh_connections[0];
        if (output_geom_plug.node().apiType() == MFn::kSkinClusterFilter)
        {
            data_.use_pivot = true;
            fn_skin_cluster.setObject(output_geom_plug.node());
            MDagPathArray influences_dag_path;
            int num_influences = fn_skin_cluster.influenceObjects(influences_dag_path);
            if (num_influences > 1)
                FAILURE("ScoWriter: particles can't be bound to more than 1 joint");

            MFnTransform joint_transform(influences_dag_path[0]);
            pos = joint_transform.getTranslation(MSpace::kTransform);
            data_.px = data_.tx - static_cast<float>(pos.x);
            data_.py = data_.ty - static_cast<float>(pos.y);
            data_.pz = data_.tz - static_cast<float>(pos.z);
        }
    }

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
        FAILURE("ScoWriter: mesh contains holes");

    // check for triangulation
    MItMeshPolygon mesh_polygon_iter(mesh_dag_path);
    for (mesh_polygon_iter.reset(); !mesh_polygon_iter.isDone(); mesh_polygon_iter.next())
    {
        if (!mesh_polygon_iter.hasValidTriangulation())
            FAILURE("ScoWriter: a poly has no valid triangulation");
    }

    // get vertices
    int num_vertices = mesh.numVertices();
    data_.num_vtxs = num_vertices;
    MFloatPointArray vertex_array;
    mesh.getPoints(vertex_array, MSpace::kWorld);
    for (int i = 0; i < num_vertices; i++)
    {
        ScoVtx vtx;
        vtx.x = vertex_array[i].x;
        vtx.y = vertex_array[i].y;
        vtx.z = vertex_array[i].z;
        data_.vertices.push_back(vtx);
    }

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
        ScoMaterial material;
        
        // get the plug for SurfaceShader
        MPlug shader_plug = MFnDependencyNode(shaders[i]).findPlug("surfaceShader");

        // get the connections to this plug
        MPlugArray plug_array;
        shader_plug.connectedTo(plug_array, true, false, &status);

        // first connection is material.
        MFnDependencyNode surface_shader(plug_array[0].node());

        material.name = surface_shader.name();

        data_.materials.push_back(material);
    }

    return MS::kSuccess;
}

} // namespace riot

