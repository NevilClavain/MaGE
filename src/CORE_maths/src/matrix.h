
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

#include "tvector.h"

namespace mage
{
	namespace core
	{
        namespace maths
        {
            class Matrix
            {
            public:

                enum class ConfigurationType
                {
                    CONFIG_UNDETERMINED,
                    CONFIG_ZERO,
                    CONFIG_IDENTITY,
                    CONFIG_TRANSLATION,
                    CONFIG_SCALING,
                    CONFIG_ROTATION,
                    CONFIG_PROJ

                };

                struct ConfigurationInfo
                {
                    ConfigurationType   type{ ConfigurationType::CONFIG_UNDETERMINED };
                    Real4Vector         values;
                };


                Matrix(void);
                ~Matrix(void) = default;

                void zero(void);

                double operator()(int p_row, int p_col) const
                {
                    return m_matrix[p_row][p_col];
                };

                double& operator()(int p_row, int p_col)
                {
                    m_configinfos.type = ConfigurationType::CONFIG_UNDETERMINED;
                    return m_matrix[p_row][p_col];
                };

                void identity(void);
                void translation(double p_x, double p_y, double p_z);
                void translation(const Real3Vector& p_pos);
                void translation(const Real4Vector& p_pos);
                void transpose(void);
                void perspective(double p_w, double p_h, double p_zn, double p_zf);
                void orthogonal(double p_w, double p_h, double p_zn, double p_zf);

                void scale(double p_sx, double p_sy, double p_sz);
                void scale(const Real3Vector& p_pos);
                void scale(const Real4Vector& p_pos);
                void clearTranslation(void);

                void rotation(const Real3Vector& p_axis, double p_angle);
                void inverse(void);
                void transform(Real4Vector* p_vec_in, Real4Vector* p_vec_out) const;

                Real3Vector getPosition() const;

                std::string dump() const;

                double* getArray(void) const
                {
                    return (double*)m_matrix;
                };

                static void matrixMult(Matrix* p_mA, Matrix* p_mB, Matrix* p_mRes);

                static Matrix buildTranslation(double p_x, double p_y, double p_z)
                {
                    Matrix t;
                    t.translation(p_x, p_y, p_z);
                    return t;
                }

                static Matrix buildTranslation(const Real3Vector& p_pos)
                {
                    Matrix t;
                    t.translation(p_pos);
                    return t;
                }


            private:

                ConfigurationInfo   m_configinfos;
                double              m_matrix[4][4];
            };
        }
	}
}

mage::core::maths::Matrix operator* (mage::core::maths::Matrix p_mA, mage::core::maths::Matrix p_mB);
