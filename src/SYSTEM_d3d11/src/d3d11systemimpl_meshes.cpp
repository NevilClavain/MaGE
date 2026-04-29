/* -*-LIC_BEGIN-*- */
/*
*
* MaGE rendering framework
* Emmanuel Chaumont Copyright (c) 2013-2026
*
* This file is part of MaGE.
*
*    MaGE is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    MaGE is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with MaGE.  If not, see <http://www.gnu.org/licenses/>.
*
*/
/* -*-LIC_END-*- */

#include <vector>
#include <omp.h>

#include "d3d11systemimpl.h"

#include "logsink.h"
#include "logconf.h"
#include "logging.h"

#include "matrixchain.h"


bool D3D11SystemImpl::createLineMeshe(const mage::LineMeshe& p_lm)
{
    DECLARE_D3D11ASSERT_VARS

    const auto resource_uid{ p_lm.getResourceUID() };

    _MAGE_DEBUG(m_localLogger, "Line meshe loading : " + resource_uid);

    if (m_lines.count(resource_uid))
    {
        _MAGE_DEBUG(m_localLogger, "Line meshe already loaded : " + resource_uid);
    }
    else
    {
        const auto nb_vertices{ p_lm.getVerticesListSize() };
        const auto nb_lines{ p_lm.getLinesListSize() };

        D3D11_SUBRESOURCE_DATA id = { 0 };

        ID3D11Buffer* vertex_buffer{ nullptr };
        ID3D11Buffer* index_buffer{ nullptr };
        ID3D11Buffer* transformer_buffer{ nullptr };

        {
            // vertex buffer creation
            const auto v{ new d3d11vertex[nb_vertices] };

            const auto vertices{ p_lm.getVertices() };

            for (size_t i = 0; i < nb_vertices; i++)
            {
                const auto vertex{ vertices[i] };

                v[i].pos.x = (float)vertex.x;
                v[i].pos.y = (float)vertex.y;
                v[i].pos.z = (float)vertex.z;

                for (size_t j = 0; j < nbTextureStages; j++)
                {
                    v[i].t[j].x = vertex.tu[j];
                    v[i].t[j].y = vertex.tv[j];
                    v[i].t[j].z = vertex.tw[j];
                    v[i].t[j].w = vertex.ta[j];
                }
            }
           
            id.pSysMem = v;
            id.SysMemPitch = 0;//sizeof( d3d11vertex );
            id.SysMemSlicePitch = 0;

            D3D11_BUFFER_DESC vertexBufferDesc = { 0 };

            vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            vertexBufferDesc.ByteWidth = nb_vertices * sizeof(d3d11vertex);
            vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            vertexBufferDesc.MiscFlags = 0;
          
            hRes = m_lpd3ddevice->CreateBuffer(&vertexBufferDesc, &id, &vertex_buffer);
            D3D11_CHECK(CreateBuffer)

            delete[] v;
        }

        {
            // index buffer creation

            const auto t{ new d3d11line[nb_lines] };

            const auto lines { p_lm.getLines() };

            for (size_t i = 0; i < nb_lines; i++)
            {
                const auto line{ lines[i] };

                t[i].vertex1 = line.at(0);
                t[i].vertex2 = line.at(1);
            }

            id.pSysMem = t;

            id.SysMemPitch = 0;
            id.SysMemSlicePitch = 0;

            D3D11_BUFFER_DESC indexBufferDesc = { 0 };

            indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            indexBufferDesc.ByteWidth = nb_lines * sizeof(d3d11line);
            indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            indexBufferDesc.MiscFlags = 0;

            hRes = m_lpd3ddevice->CreateBuffer(&indexBufferDesc, &id, &index_buffer);
            D3D11_CHECK(CreateBuffer)

            delete[] t;
        }

        // TRANSFORMERS buffer creation
        {
            D3D11_BUFFER_DESC transformersBufferDesc = { 0 };

            transformersBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            
            transformersBufferDesc.ByteWidth = nbMaxTransformersInstances * sizeof(d3d11transformers);
            transformersBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            transformersBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            transformersBufferDesc.MiscFlags = 0;

            hRes = m_lpd3ddevice->CreateBuffer(&transformersBufferDesc, nullptr, &transformer_buffer);
            D3D11_CHECK(CreateBuffer)
        }

        m_lines[resource_uid] = { vertex_buffer, index_buffer, transformer_buffer, nbMaxTransformersInstances, nb_vertices, nb_lines };
    }

    _MAGE_DEBUG(m_localLogger, "Line meshe loading SUCCESS : " + resource_uid);
	return true;
}

void D3D11SystemImpl::setLineMeshe(const std::string& p_md5)
{
    if (!m_lines.count(p_md5))
    {
        _EXCEPTION("unknown line meshes :" + p_md5)
    }

    if (m_currentMeshe != p_md5)
    {
        const auto lmData{ m_lines.at(p_md5) };

        UINT strides[2] = { sizeof(d3d11vertex), sizeof(d3d11transformers) };
        UINT offsets[2] = { 0, 0 };
        ID3D11Buffer* buffers[2] = { lmData.vertex_buffer, lmData.transforms_buffer };
        m_lpd3ddevcontext->IASetVertexBuffers(0, 2, buffers, strides, offsets);

        m_lpd3ddevcontext->IASetIndexBuffer(lmData.index_buffer, DXGI_FORMAT_R32_UINT, 0);

        m_next_nbvertices = lmData.nb_vertices;
        m_next_nblines = lmData.nb_primitives;

        m_currentMeshe = p_md5;
    }
}


//void D3D11SystemImpl::destroyLineMeshe(const std::string& p_md5)
//{
//    if (!m_lines.count(p_md5))
//    {
//        _EXCEPTION("unknown line meshe :" + p_md5)
//    }
//    const auto lmData{ m_lines.at(p_md5) };
//
//    lmData.vertex_buffer->Release();
//    lmData.index_buffer->Release();
//
//    m_lines.erase(p_md5);
//    _MAGE_DEBUG(m_localLogger, "Line meshe release SUCCESS : " + p_md5);
//}

bool D3D11SystemImpl::createTriangleMeshe(const mage::TriangleMeshe& p_tm)
{
    DECLARE_D3D11ASSERT_VARS

    const auto resource_uid{ p_tm.getResourceUID()};

    _MAGE_DEBUG(m_localLogger, "Triangle meshe loading : " + resource_uid);

    if (m_triangles.count(resource_uid))
    {
        _MAGE_DEBUG(m_localLogger, "Triangle meshe already loaded : " + resource_uid);
    }
    else
    {
        const auto nb_vertices{ p_tm.getVerticesListSize() };
        const auto nb_triangles{ p_tm.getTrianglesListSize() };

        D3D11_SUBRESOURCE_DATA id = { 0 };

        ID3D11Buffer* vertex_buffer{ nullptr };
        ID3D11Buffer* index_buffer{ nullptr };
        ID3D11Buffer* transformer_buffer{ nullptr };

        {
            // vertex buffer creation
            const auto v{ new d3d11vertex[nb_vertices] };

            const auto vertices{ p_tm.getVertices() };

            for (size_t i = 0; i < nb_vertices; i++)
            {
                const auto vertex{ vertices[i] };

                v[i].pos.x = (float)vertex.x;
                v[i].pos.y = (float)vertex.y;
                v[i].pos.z = (float)vertex.z;

                v[i].normale.x = (float)vertex.nx;
                v[i].normale.y = (float)vertex.ny;
                v[i].normale.z = (float)vertex.nz;

                v[i].tangent.x = (float)vertex.tx;
                v[i].tangent.y = (float)vertex.ty;
                v[i].tangent.z = (float)vertex.tz;

                v[i].binormale.x = (float)vertex.bx;
                v[i].binormale.y = (float)vertex.by;
                v[i].binormale.z = (float)vertex.bz;

                for (size_t j = 0; j < nbTextureStages; j++)
                {
                    v[i].t[j].x = vertex.tu[j];
                    v[i].t[j].y = vertex.tv[j];
                    v[i].t[j].z = vertex.tw[j];
                    v[i].t[j].w = vertex.ta[j];
                }
            }

            id.pSysMem = v;
            id.SysMemPitch = 0;//sizeof( d3d11vertex );
            id.SysMemSlicePitch = 0;

            D3D11_BUFFER_DESC vertexBufferDesc = { 0 };

            vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            vertexBufferDesc.ByteWidth = nb_vertices * sizeof(d3d11vertex);
            vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            vertexBufferDesc.MiscFlags = 0;

            hRes = m_lpd3ddevice->CreateBuffer(&vertexBufferDesc, &id, &vertex_buffer);
            D3D11_CHECK(CreateBuffer)

            delete[] v;
        }

        {
            // index buffer creation

            const auto t{ new d3d11triangle[nb_triangles] };

            const auto triangles{ p_tm.getTriangles() };

            for (size_t i = 0; i < nb_triangles; i++)
            {
                const auto triangle{ triangles[i] };

                t[i].vertex1 = triangle.at(0);
                t[i].vertex2 = triangle.at(1);
                t[i].vertex3 = triangle.at(2);
            }

            id.pSysMem = t;

            id.SysMemPitch = 0;
            id.SysMemSlicePitch = 0;

            D3D11_BUFFER_DESC indexBufferDesc = { 0 };

            indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            indexBufferDesc.ByteWidth = nb_triangles * sizeof(d3d11triangle);
            indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            indexBufferDesc.MiscFlags = 0;

            hRes = m_lpd3ddevice->CreateBuffer(&indexBufferDesc, &id, &index_buffer);
            D3D11_CHECK(CreateBuffer)

            delete[] t;
        }

        // TRANSFORMERS buffer creation
        {
            D3D11_BUFFER_DESC transformersBufferDesc = { 0 };

            transformersBufferDesc.Usage = D3D11_USAGE_DYNAMIC;

            transformersBufferDesc.ByteWidth = nbMaxTransformersInstances * sizeof(d3d11transformers);
            transformersBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            transformersBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            transformersBufferDesc.MiscFlags = 0;

            hRes = m_lpd3ddevice->CreateBuffer(&transformersBufferDesc, nullptr, &transformer_buffer);
            D3D11_CHECK(CreateBuffer)
        }

        m_triangles[resource_uid] = { vertex_buffer, index_buffer, transformer_buffer, nbMaxTransformersInstances, nb_vertices, nb_triangles };
    }

    _MAGE_DEBUG(m_localLogger, "Triangle meshe loading SUCCESS : " + resource_uid);
    return true;

}

void D3D11SystemImpl::setTriangleMeshe(const std::string& p_resource_uid)
{
    if (!m_triangles.count(p_resource_uid))
    {
        _EXCEPTION("unknown triangle meshes :" + p_resource_uid)
    }

    if (m_currentMeshe != p_resource_uid)
    {
        const auto tmData{ m_triangles.at(p_resource_uid) };

        UINT strides[2] = { sizeof(d3d11vertex), sizeof(d3d11transformers) };
        UINT offsets[2] = { 0, 0 };
        ID3D11Buffer* buffers[2] = { tmData.vertex_buffer, tmData.transforms_buffer };
        m_lpd3ddevcontext->IASetVertexBuffers(0, 2, buffers, strides, offsets);


        m_lpd3ddevcontext->IASetIndexBuffer(tmData.index_buffer, DXGI_FORMAT_R32_UINT, 0);

        m_next_nbvertices = tmData.nb_vertices;
        m_next_nbtriangles = tmData.nb_primitives;

        m_currentMeshe = p_resource_uid;
    }
}

/*
void D3D11SystemImpl::destroyTriangleMeshe(const std::string& p_resource_uid)
{
    if (!m_triangles.count(p_resource_uid))
    {
        _EXCEPTION("unknown triangle meshe :" + p_resource_uid)
    }
    const auto tmData{ m_triangles.at(p_resource_uid) };

    tmData.vertex_buffer->Release();
    tmData.index_buffer->Release();

    m_triangles.erase(p_resource_uid);
    _MAGE_DEBUG(m_localLogger, "Triangle meshe release SUCCESS : " + p_resource_uid);
}
*/


void D3D11SystemImpl::forceCurrentMeshe()
{
    if (m_triangles.count(m_currentMeshe))
    {
        const auto tmData{ m_triangles.at(m_currentMeshe) };

        UINT strides[2] = { sizeof(d3d11vertex), sizeof(d3d11transformers) };
        UINT offsets[2] = { 0, 0 };
        ID3D11Buffer* buffers[2] = { tmData.vertex_buffer, tmData.transforms_buffer };
        m_lpd3ddevcontext->IASetVertexBuffers(0, 2, buffers, strides, offsets);

        m_lpd3ddevcontext->IASetIndexBuffer(tmData.index_buffer, DXGI_FORMAT_R32_UINT, 0);

        m_next_nbvertices = tmData.nb_vertices;
        m_next_nbtriangles = tmData.nb_primitives;
    }
    else if (m_lines.count(m_currentMeshe))
    {
        const auto lmData{ m_lines.at(m_currentMeshe) };

        UINT strides[2] = { sizeof(d3d11vertex), sizeof(d3d11transformers) };
        UINT offsets[2] = { 0, 0 };
        ID3D11Buffer* buffers[2] = { lmData.vertex_buffer, lmData.transforms_buffer };
        m_lpd3ddevcontext->IASetVertexBuffers(0, 2, buffers, strides, offsets);

        m_lpd3ddevcontext->IASetIndexBuffer(lmData.index_buffer, DXGI_FORMAT_R32_UINT, 0);

        m_next_nbvertices = lmData.nb_vertices;
        m_next_nblines = lmData.nb_primitives;
    }
}

bool D3D11SystemImpl::updateMesheTransformers(const MesheData& p_meshe_data, const std::vector<mage::core::maths::Matrix*>& p_worlds,
    const mage::core::maths::Matrix& p_view, const mage::core::maths::Matrix& p_proj,
    const mage::core::maths::Matrix& p_view2, const mage::core::maths::Matrix& p_proj2)
{

    DECLARE_D3D11ASSERT_VARS

    mage::core::maths::Matrix inv;
    inv.identity();
    inv(2, 2) = -1.0;
    const auto final_view{ p_view * inv };
    const auto final_view2{ p_view2 * inv };

    std::vector<d3d11transformers> instances(p_worlds.size());

    if (instances.size() > p_meshe_data.transforms_buffer_size)
    {
        // grow buffer

        _asm nop
    }

    // looks like fps is degraded with omp ! :(
    //#pragma omp parallel for default(shared) schedule(static)
    for (int i = 0; i < static_cast<int>(p_worlds.size()); ++i)
    {
        mage::transform::MatrixChain chain;

        chain.pushMatrix(p_proj);
        chain.pushMatrix(final_view);
        chain.pushMatrix(*p_worlds[i]);
        chain.buildResult();
        auto result{ chain.getResultTransform() };

        mage::transform::MatrixChain chain2;

        chain2.pushMatrix(p_proj2);
        chain2.pushMatrix(final_view2);
        chain2.pushMatrix(*p_worlds[i]);
        chain2.buildResult();
        auto result2{ chain2.getResultTransform() };

        d3d11transformers tr;
        tr.wordlViewProj = convertMatrixToXMFloat44(result);
        tr.world = convertMatrixToXMFloat44(*p_worlds[0]);
        tr.wordlView2Proj2 = convertMatrixToXMFloat44(result2);

        instances[i] = tr;
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    hRes = m_lpd3ddevcontext->Map(p_meshe_data.transforms_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    D3D11_CHECK(Map)

    memcpy(mapped.pData, instances.data(), sizeof(d3d11transformers) * instances.size());

    m_lpd3ddevcontext->Unmap(p_meshe_data.transforms_buffer, 0);
}
