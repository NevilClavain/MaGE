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

#pragma once

#include <string>

#include "matrix.h"
#include "datacloud.h"
#include "tvector.h"

namespace mage
{
	namespace transform
	{
		template<typename T>
		struct IMatrixSource
		{
		public:
			virtual T getValue() const = 0;
		};

		template<typename T>
		struct DirectValueMatrixSource : public IMatrixSource<T>
		{
			static_assert(std::is_arithmetic<T>::value || mage::core::maths::is_tvector<T>::value, "Need an arithmetic or tvector type");
		
		public:
			DirectValueMatrixSource(T p_value) : 
			m_value(p_value)
			{
			}
			
			inline T	getValue() const;
			inline void updateValue(T p_value);

		private:
			T m_value;

		};

		template<typename T>
		T DirectValueMatrixSource<T>::getValue() const
		{
			return m_value;
		}

		template<typename T>
		void DirectValueMatrixSource<T>::updateValue(T p_value)
		{
			return m_value;
		}

		// dynamically build matrix with values taken from : datacloud or sync var components or hardcoded vals 
		struct MatrixFactory
		{
		public:

			MatrixFactory() = default;
			~MatrixFactory() = default;

			void setXSource(IMatrixSource<double>* p_source);
			void setYSource(IMatrixSource<double>* p_source);
			void setZSource(IMatrixSource<double>* p_source);
			void setWSource(IMatrixSource<double>* p_source);

			void setXYZWSource(IMatrixSource<core::maths::Real4Vector>* p_source);
			void setXYZSource(IMatrixSource<core::maths::Real3Vector>* p_source);

			core::maths::Matrix getResult();

		private:

			IMatrixSource<core::maths::Real3Vector>*	m_xyz_source{ nullptr };
			IMatrixSource<core::maths::Real4Vector>*	m_xyzw_source{ nullptr };

			double										m_x{ 0 };
			IMatrixSource<double>*						m_x_source{ nullptr };

			double										m_y{ 0 };
			IMatrixSource<double>*						m_y_source{ nullptr };

			double										m_z{ 0 };
			IMatrixSource<double>*						m_z_source{ nullptr };

			double										m_w{ 0 };
			IMatrixSource<double>*						m_w_source{ nullptr };
		};
	}
}
