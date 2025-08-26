/* -*-LIC_BEGIN-*- */
/*
*
* MaGE rendering framework
* Emmanuel Chaumont Copyright (c) 2013-2025
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

#include "matrixfactory.h"
#include "exceptions.h"

using namespace mage;
using namespace mage::transform;
using namespace mage::core::maths;

void MatrixFactory::setXSource(IMatrixSource<double>* p_source)
{
	m_x_source = p_source;
}

void MatrixFactory::setYSource(IMatrixSource<double>* p_source)
{
	m_y_source = p_source;
}

void MatrixFactory::setZSource(IMatrixSource<double>* p_source)
{
	m_z_source = p_source;
}

void MatrixFactory::setWSource(IMatrixSource<double>* p_source)
{
	m_w_source = p_source;
}

void MatrixFactory::setXYZWSource(IMatrixSource<core::maths::Real4Vector>* p_source)
{
	m_xyzw_source = p_source;
}

void MatrixFactory::setXYZSource(IMatrixSource<core::maths::Real3Vector>* p_source)
{
	m_xyz_source = p_source;
}


Matrix MatrixFactory::getResult()
{
	// collect and update current values

	if (m_xyzw_source)
	{
		const auto vec{ m_xyzw_source->getValue() };

		m_x = vec[0];
		m_y = vec[1];
		m_z = vec[2];
		m_w = vec[3];
	}
	else if (m_xyz_source)
	{
		const auto vec{ m_xyz_source->getValue() };

		m_x = vec[0];
		m_y = vec[1];
		m_z = vec[2];

		if (!m_w_source)
		{
			_EXCEPTION("Missing source fox w");
		}
		m_w = m_w_source->getValue();

	}
	else
	{
		if (!m_x_source)
		{
			_EXCEPTION("Missing source fox x");
		}
		m_x = m_x_source->getValue();

		if (!m_y_source)
		{
			_EXCEPTION("Missing source fox y");
		}
		m_y = m_y_source->getValue();

		if (!m_z_source)
		{
			_EXCEPTION("Missing source fox z");
		}
		m_z = m_z_source->getValue();

		if (!m_w_source)
		{
			_EXCEPTION("Missing source fox w");
		}
		m_w = m_w_source->getValue();
	}

	// build matrix and return

	Matrix result;
	return result;
}

