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

#pragma once

#include <string>
#include <unordered_map>
#include <functional>

#include "matrix.h"
#include "datacloud.h"
#include "tvector.h"
#include "syncvariable.h"

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

		struct SyncVarValueMatrixSource : public IMatrixSource<double>
		{
		public:
			SyncVarValueMatrixSource(core::SyncVariable* p_syncvar) :
			m_syncvar(p_syncvar)
			{
			}

			inline double getValue() const;

		private:
	
			core::SyncVariable* m_syncvar{ nullptr };

		};

		double SyncVarValueMatrixSource::getValue() const
		{
			return m_syncvar->value;
		}

		template<typename T>
		struct DatacloudValueMatrixSource : public IMatrixSource<T>
		{
			static_assert(std::is_arithmetic<T>::value || mage::core::maths::is_tvector<T>::value, "Need an arithmetic or tvector type");

		public:
			DatacloudValueMatrixSource(const std::string& p_valueId) :
			m_valueId(p_valueId)
			{
			}

			inline T getValue() const;		
		private:
			std::string m_valueId;

		};

		template<typename T>
		T DatacloudValueMatrixSource<T>::getValue() const
		{
			const auto dataCloud{ mage::rendering::Datacloud::getInstance() };

			return dataCloud->readDataValue<T>(m_valueId);
		}

		// dynamically build matrix with values taken from : datacloud or sync var components or hardcoded vals 
		struct MatrixFactory
		{
		public:

			using buildFunc = std::function <core::maths::Matrix(double p_x, double p_y, double p_z, double p_w)>;

			MatrixFactory(const std::string& p_build_type, double p_x = 0.0, double p_y = 0.0, double p_z = 0.0, double p_w = 0.0);
			~MatrixFactory() = default;

			void setXSource(IMatrixSource<double>* p_source);
			void setYSource(IMatrixSource<double>* p_source);
			void setZSource(IMatrixSource<double>* p_source);
			void setWSource(IMatrixSource<double>* p_source);

			void setXYZWSource(IMatrixSource<core::maths::Real4Vector>* p_source);
			void setXYZSource(IMatrixSource<core::maths::Real3Vector>* p_source);

			core::maths::Matrix getResult();

			static void registerBuildFunc(const std::string& p_id, const buildFunc& p_func);

		private:

			IMatrixSource<core::maths::Real3Vector>*	m_xyz_source{ nullptr };
			IMatrixSource<core::maths::Real4Vector>*	m_xyzw_source{ nullptr };

			double										m_x;
			IMatrixSource<double>*						m_x_source{ nullptr };

			double										m_y;
			IMatrixSource<double>*						m_y_source{ nullptr };

			double										m_z;
			IMatrixSource<double>*						m_z_source{ nullptr };

			double										m_w;
			IMatrixSource<double>*						m_w_source{ nullptr };

			std::string									m_build_type;

			static std::unordered_map 
				<std::string, buildFunc>				m_build_funcs;
		};
	}
}
