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

#include "d3d11systemimpl.h"
#include "aspects.h"

D3D11SystemImpl::D3D11SystemImpl() :
m_localLogger("D3D11System", mage::core::logger::Configuration::getInstance())
{
}

mage::core::logger::Sink& D3D11SystemImpl::logger()
{
	return m_localLogger;
}

DirectX::XMFLOAT4X4 D3D11SystemImpl::convertMatrixToXMFloat44(const mage::core::maths::Matrix& p_mat)
{
	DirectX::XMFLOAT4X4 xm_mat;

    xm_mat._11 = p_mat(0, 0);
    xm_mat._12 = p_mat(0, 1);
    xm_mat._13 = p_mat(0, 2);
    xm_mat._14 = p_mat(0, 3);

    xm_mat._21 = p_mat(1, 0);
    xm_mat._22 = p_mat(1, 1);
    xm_mat._23 = p_mat(1, 2);
    xm_mat._24 = p_mat(1, 3);

    xm_mat._31 = p_mat(2, 0);
    xm_mat._32 = p_mat(2, 1);
    xm_mat._33 = p_mat(2, 2);
    xm_mat._34 = p_mat(2, 3);

    xm_mat._41 = p_mat(3, 0);
    xm_mat._42 = p_mat(3, 1);
    xm_mat._43 = p_mat(3, 2);
    xm_mat._44 = p_mat(3, 3);

	return xm_mat;
}