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

#include <SknWriter.h>

#include <set>

#include <maya/MGlobal.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MDoubleArray.h>
#include <maya/MIntArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MPointArray.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>

#include <maya_misc.h>
#include <SknData.hpp>

namespace riot {

MStatus SknWriter::write(ostream& file)
{
    data_.switchHand();

    // magic
    int magic = 0x00112233;
    file.write(reinterpret_cast<char*>(&magic), 4);

    // set version
    USHORT version = 1;
    file.write(reinterpret_cast<char*>(&version), 2);

    // set num obj
    USHORT num_objects = 1;
    file.write(reinterpret_cast<char*>(&num_objects), 2);

    // set materials
    int num_materials = static_cast<int>(data_.materials.size());
    file.write(reinterpret_cast<char*>(&num_materials), 4);
    for (int i = 0; i < num_materials; i++)
    {
        SknMaterial material = data_.materials.at(i);
        file.write(reinterpret_cast<char*>(&material), SknMaterial::kSizeInFile);
    }

    if (num_materials > 2 || num_materials < 1)
        FAILURE("SknWriter: num_materials < 1 || > 2");

    // set nums
    int num_indices = data_.num_indices;
    file.write(reinterpret_cast<char*>(&num_indices), 4);
    int num_vertices = data_.num_vtxs;
    file.write(reinterpret_cast<char*>(&num_vertices), 4);

    if (num_indices % 3 != 0)
        FAILURE("SknWriter: num_indices % 3 != 0 ...");


    // set indices
    for (int i = 0; i < num_indices; i++)
    {
        USHORT indice = data_.indices.at(i);
        file.write(reinterpret_cast<char*>(&indice), 2);
    }

    // set vertices
    for (int i = 0; i < num_vertices; i++)
    {
        SknVtx vtx = data_.vertices.at(i);
        file.write(reinterpret_cast<char*>(&vtx), SknVtx::kSizeInFile);
    }

    /*
    // set endtab
    if (version == 2)
        file.write(reinterpret_cast<char*>((data_.endTab)), 12);
    */

    return MS::kSuccess;
}

MStatus SknWriter::dumpData(SklData* skl_data)
{
    MStatus status;
    MDagPath dag_path;
    MDagPath mesh_dag_path;

    MSelectionList selection_list;
    if (MStatus::kSuccess != MGlobal::getActiveSelectionList(selection_list))
        FAILURE("SknWriter: MGlobal::getActiveSelectionList()");

    MItSelectionList it_selection_list(selection_list, MFn::kMesh, &status);    
    if (status != MStatus::kSuccess)
        FAILURE("SknWriter: it_selection_list()");

    if (it_selection_list.isDone())
        FAILURE("SknWriter: no mesh selected!");

    it_selection_list.getDagPath(mesh_dag_path);
    it_selection_list.next();

    if (!it_selection_list.isDone())
        FAILURE("SknWriter: more than one mesh selected!");

    MFnMesh mesh(mesh_dag_path);

    data_.version = 1;

    // find skincluster
    MPlug in_mesh_plug = mesh.findPlug("inMesh");
    MPlugArray in_mesh_connections;
    in_mesh_plug.connectedTo(in_mesh_connections, true, false);
    if (in_mesh_connections.length() == 0)
        FAILURE("SknWriter: failed to find the mesh's skinCluster.");
    MPlug output_geom_plug = in_mesh_connections[0];
    MFnSkinCluster fn_skin_cluster(output_geom_plug.node());
    MDagPathArray influences_dag_path;
    int num_influences = fn_skin_cluster.influenceObjects(influences_dag_path);
    if (num_influences > SklData::kMaxIndices)
    {
        int toRemove = num_influences - SklData::kMaxIndices;
        FAILURE(MString("SknWriter: too much bound bones, plz remove ") + toRemove + " influence(s)");
    }
    
    // get skl indices by influence index
    MIntArray skl_indices_by_influence_index(num_influences);
    for (int i = 0; i < num_influences; i++)
    {
        MDagPath jointPath = influences_dag_path[i];
        int j = 0;
        for (; j < skl_data->num_bones; j++)
        {
            if (jointPath == skl_data->joints[j])
                break;
        }
        if (j == skl_data->num_bones)
            FAILURE("SknWriter: unable to find a bound bone in the skeleton data_.\
                        this error should not happen!");
        
        skl_indices_by_influence_index[i] = j;
    }

    // will be used for vtx indices
    MIntArray mask_influence_index(num_influences);

    if (skl_data->num_bones <= SklData::kMaxIndices)
    {
        // case type 1
        skl_data->version = 1;
        mask_influence_index = skl_indices_by_influence_index;
    }
    else
    {
        // case type 2
        skl_data->version = 2;
        skl_data->num_indices = num_influences;
        for (int i = 0; i < num_influences; i++)
        {
            mask_influence_index[i] = i;
            skl_data->skn_indices.append(skl_indices_by_influence_index[i]);
        }
    }

    int instance_num = 0;
    if (mesh_dag_path.isInstanced())
        instance_num = mesh_dag_path.instanceNumber();

    MObjectArray shaders;
    MIntArray poly_shader_indices;
    mesh.getConnectedShaders(instance_num, shaders, poly_shader_indices);
    int shader_count = shaders.length();

    int num_vertices = mesh.numVertices();

    if(shader_count > 2)
    {
        FAILURE(MString("shaders for this mesh : ") + shader_count + ", this is more than allowed (2)");
    }
    else
        MGlobal::displayInfo(MString("shaders for this mesh : ") + shader_count);
        
    // check for holes
    MIntArray hole_info_array;
    MIntArray hole_vertex_array;
    mesh.getHoles(hole_info_array, hole_vertex_array);
    if (hole_info_array.length() != 0)
        FAILURE("SknWriter: mesh contains holes");

    MIntArray shader_by_vertex(num_vertices, -1);

    // check if shaders don't share vertices and btw check for triangulation
    MItMeshPolygon mesh_polygon_iter(mesh_dag_path);
    for (mesh_polygon_iter.reset(); !mesh_polygon_iter.isDone(); mesh_polygon_iter.next())
    {
        if (!mesh_polygon_iter.hasValidTriangulation())
            FAILURE("SknWriter: a poly has no valid triangulation");

        int polyIndex = mesh_polygon_iter.index();
        int shaderIndex = poly_shader_indices[polyIndex];
        
        MIntArray vertices;
        mesh_polygon_iter.getVertices(vertices);
        if (shaderIndex == -1)
            FAILURE("SknWriter: some polygons have no shader");
        int vertices_length = static_cast<int>(vertices.length());
        for (int i = 0; i < vertices_length; i++)
        {
            if (shader_count > 1 && shader_by_vertex[vertices[i]] != -1 && shaderIndex != shader_by_vertex[vertices[i]])
                FAILURE("SknWriter: some vertices are shared by different shaders");
            
            shader_by_vertex[vertices[i]] = shaderIndex;
        }
    }

    // get weights
    MFnSingleIndexedComponent fn_comp;
    MObject vtx_comp = fn_comp.create(MFn::kMeshVertComponent);
    MIntArray group_vtx_indices(num_vertices);
    for (int i = 0; i < num_vertices; i++)
        group_vtx_indices[i] = i; //TODO:FIX
    fn_comp.addElements(group_vtx_indices);
    MDoubleArray weights;
    int influence_count;
    unsigned int influence_count_tmp;
    fn_skin_cluster.getWeights(mesh_dag_path, vtx_comp, weights, influence_count_tmp);
    influence_count = static_cast<int>(influence_count_tmp);

    // check weights (if more than 5 ... "finish him !")
    for (int i = 0; i < num_vertices; i++)
    {
        int num = 0;
        double sum = 0;
        for (int j = 0; j < influence_count; j++)
        {
            double weight = weights[i * influence_count + j];
            if (weight != 0)
            {
                num++;
                sum += weight;
            }
        }

        if (num > 4)
            FAILURE("SknWriter: plz set max influences to 4 ...");

        // normalize
        for (int j = 0; j < influence_count; j++)
        {
            weights[i * influence_count + j] /= sum;
        }
    }

    // create stuff for materials :)
    std::vector<MIntArray> shader_vertex_indices; // maya index per data index
                                                // the size is numFinalVertices
    std::vector<std::vector<SknVtx>> shader_vtxs;
    std::vector<MIntArray> shader_triangles;
    for (int i = 0; i < shader_count; i++)
    {
        shader_vertex_indices.push_back(MIntArray());
        std::vector<SknVtx> vtxs;
        shader_vtxs.push_back(vtxs);
        shader_triangles.push_back(MIntArray());
        data_.materials.push_back(SknMaterial());
    }

    // check for any vertex with no shader
    // and btw fill stuff for materials
    int num_useful_vertices = num_vertices;
    MItMeshVertex mesh_vertices_iter(mesh_dag_path);
    for (mesh_vertices_iter.reset(); !mesh_vertices_iter.isDone(); mesh_vertices_iter.next())
    {
        int index = mesh_vertices_iter.index();
        int shader = shader_by_vertex[index];
        if (shader == -1)
        {
            num_useful_vertices--;
            continue; // alone vertex
        }

        // create and store vtxs for each UVs of the current vertex
        SknVtx vtx;
        MPoint pos = mesh_vertices_iter.position(MSpace::kWorld);
        vtx.x = static_cast<float>(pos.x);
        vtx.y = static_cast<float>(pos.y);
        vtx.z = static_cast<float>(pos.z);
        MVectorArray normals;
        mesh_vertices_iter.getNormals(normals);
        // average that
        double3 normal = {0, 0, 0};
        int numNormals = normals.length();
        for (int i = 0; i < numNormals; i++)
        {
                normal[0] += normals[i].x;
                normal[1] += normals[i].y;
                normal[2] += normals[i].z;
        }
        vtx.normal[0] = static_cast<float>(normal[0] / numNormals);
        vtx.normal[1] = static_cast<float>(normal[1] / numNormals);
        vtx.normal[2] = static_cast<float>(normal[2] / numNormals);

        // search for influences
        int found = 0;
        for (int j = 0; j < influence_count && found < 4; j++)
        {
            double weight = weights[index * influence_count + j];
            if (weight != 0)
            {
                vtx.skn_indices[found] = static_cast<char>(mask_influence_index[j]);
                vtx.weights[found] = static_cast<float>(weight);
                found++;
            }
        }

        // get unique UVs
        MIntArray uv_indices;
        mesh_vertices_iter.getUVIndices(uv_indices);

        if (uv_indices.length())
        {
            std::set<int> seen;
            int uv_indices_length = static_cast<int>(uv_indices.length());
            for (int j = 0; j < uv_indices_length; j++)
            {
                int uv_index = uv_indices[j];
                if (seen.find(uv_index) != seen.end())
                    continue;

                seen.insert(uv_index);
                float U, V;
                mesh.getUV(uv_index, U, V);
                vtx.U = U;
                vtx.V = 1 - V; // flip it :)
                vtx.uv_index = uv_index;
                shader_vtxs.at(shader).push_back(vtx);
                shader_vertex_indices.at(shader).append(index);
            }
        }
        else
            FAILURE("SknWriter: some vertices have no UVs");
    }

    // create the id converter from maya index to data index
    // since for each vtx its duplicates are next to it
    // we are gonna choose the index of the first vtx.
    // Then we fill the data_.vertices .
    int curID = 0;
    MIntArray data_indices(num_vertices, -1);
    for (int i = 0; i < shader_count; i++)
    {
        const MIntArray& vertex_indices = shader_vertex_indices.at(i);
        std::vector<riot::SknVtx>& vtxs = shader_vtxs.at(i);
        int vertex_indices_length = static_cast<int>(vertex_indices.length());
        for (int j = 0; j < vertex_indices_length; j++)
        {
            int index = vertex_indices[j];
            if (data_indices[index] == -1) // throw duplicates
            {
                data_indices[index] = curID;
                vtxs.at(j).dupe_data_index = curID;
            }
            else
            {
                vtxs.at(j).dupe_data_index = data_indices[index];
            }
            curID++;
            //MGlobal::displayInfo(MString("plopi : ") + shader_vtxs.at(i).at(j).dupe_data_index);
        }

        data_.vertices.insert(
            data_.vertices.end(),
            shader_vtxs.at(i).begin(),
            shader_vtxs.at(i).end()
        ); // insert at the end
    }

    // get triangles and save their file indices by material.
    for (mesh_polygon_iter.reset(); !mesh_polygon_iter.isDone(); mesh_polygon_iter.next())
    {
        int polyIndex = mesh_polygon_iter.index();
        int shaderIndex = poly_shader_indices[polyIndex];

        MIntArray indices;
        MPointArray points;
        mesh_polygon_iter.getTriangles(points, indices);

        if (mesh_polygon_iter.hasUVs())
        {
            MIntArray vertices;
            mesh_polygon_iter.getVertices(vertices);

            MIntArray new_indices(indices.length(), -1);
            // convert indices using UV indices
            int data_vertices_size = static_cast<int>(data_.vertices.size());
            int indices_length = static_cast<int>(indices.length());
            int vertices_length = static_cast<int>(vertices.length());
            for (int i = 0; i < vertices_length; i++)
            {
                int uv_index;
                mesh_polygon_iter.getUVIndex(i, uv_index);

                int data_index = data_indices[vertices[i]];
                if (data_index == -1)
                    FAILURE("SknWriter: that error should not happen, please report to thispawn.");
                if (data_index >= data_vertices_size)
                    FAILURE(MString("SknWriter: that error should not happen, please report to thispawn. ") + data_index);

                for (int j = data_index; j < data_vertices_size; j++)
                {
                    if (data_.vertices.at(j).dupe_data_index != data_index)
                    {
                        FAILURE("SknWriter: can't find the corresponding faceVertex in the data, \n" \
                                              "this error should not happen, contact ThiSpawn about this. \n");
                    }

                    if (data_.vertices.at(j).uv_index == uv_index)
                    {
                        for (int k = 0; k < indices_length; k++)
                            if (indices[k] == vertices[i])
                                new_indices[k] = j;
                        break;
                    }
                }
            }

            int new_indices_length = static_cast<int>(new_indices.length());
            for (int i = 0; i < new_indices_length; i++)
            {
                if (new_indices[i] == -1)
                    FAILURE("SknWriter: one faceVertex is not part of the face vertices oO");
                shader_triangles.at(shaderIndex).append(new_indices[i]);
            }
        }
        else
        {
            // use the vtx with shared UV
            int indices_length = static_cast<int>(indices.length());
            for (int i = 0; i < indices_length; i++)
            {
                int data_index = data_indices[indices[i]];
                if (data_index == -1)
                    FAILURE("SknWriter: that error should not happen, please report to thispawn.");
                shader_triangles.at(shaderIndex).append(data_indices[indices[i]]);
            }
        }
    }

    // fill materials
    int indiceOffset = 0;
    int vertexOffset = 0;
    for (int i = 0; i < shader_count; i++)
    {
        SknMaterial& material = data_.materials.at(i);
        material.startIndex = indiceOffset;
        material.startVertex = vertexOffset;
        int num_indices = shader_triangles.at(i).length();
        int num_vertices = shader_vertex_indices.at(i).length();
        material.num_indices = num_indices;
        material.num_vertices = num_vertices;
        indiceOffset += num_indices;
        vertexOffset += num_vertices;

        for (int j = 0; j < num_indices; j++)
            data_.indices.push_back((USHORT)shader_triangles[i][j]);
        
        // get the plug for SurfaceShader
        MPlug shader_plug = MFnDependencyNode(shaders[i]).findPlug("surfaceShader");
        
        // get the connections to this plug
        MPlugArray plug_array;
        shader_plug.connectedTo(plug_array, true, false, &status);

        // first connection is material.
        MFnDependencyNode surface_shader(plug_array[0].node());

        strcpy_s(material.name, SknMaterial::kNameLen, surface_shader.name().asChar());
    }
    data_.num_indices = indiceOffset;
    data_.num_vtxs = vertexOffset;

    return MS::kSuccess;
}

} // namespace riot

