
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

#include "tvector.h"

namespace mage
{
    namespace core
    {
        namespace maths
        {
            static constexpr double pi{ 3.141592653589793 };


			static inline double		square(double a) { return a * a; };
			static inline int			floor(double a) { return ((int)a - (a < 0 && a != (int)a)); };
			static inline int			ceiling(double a) { return ((int)a + (a > 0 && a != (int)a)); };

			static inline double		getMin(double a, double b) { return (a < b ? a : b); };
			static inline double		getMax(double a, double b) { return (a > b ? a : b); };
			static inline double		abs(double a) { return (a < 0 ? -a : a); };
			static inline double		clamp(double a, double b, double x) { return (x < a ? a : (x > b ? b : x)); };
			static inline double		lerp(double a, double b, double x) { return a + x * (b - a); };
			static inline double		cubic(double a) { return a * a * (3 - 2 * a); };
			static inline double		pulse(double a, double b, double x) { return (double)((x >= a) - (x >= b)); };
			static inline double		gamma(double a, double g) { return pow(a, 1 / g); };
			static inline double		expose(double l, double k) { return (1 - exp(-l * k)); };
			static inline double		degToRad(double ang) { return ((ang * pi) / 180.0); };
			static inline double		radToDeg(double ang) { return ((ang * 180.0) / pi); };

			template<typename Vector>
			static inline void	sphericaltoCartesian(const Vector& p_in, Vector& p_out)
			{
				p_out[2] = (p_in[0] * cos(p_in[2]) * cos(p_in[1]));
				p_out[0] = (p_in[0] * cos(p_in[2]) * sin(p_in[1]));
				p_out[1] = (p_in[0] * sin(p_in[2]));
			}

			template<typename Vector>
			static inline void	cartesiantoSpherical(Vector& p_in, Vector& p_out)
			{
				// cas particulier
				if (p_in[1] > 0.0 && 0.0 == p_in[0] && 0.0 == p_in[2])
				{
					p_out[0] = p_in[1];
					p_out[2] = pi / 2.0;
					p_out[1] = 0.0;
					return;
				}
				else if (p_in[1] < 0.0 && 0.0 == p_in[0] && 0.0 == p_in[2])
				{
					p_out[0] = -p_in[1];
					p_out[2] = -pi / 2.0;
					p_out[1] = 0.0;
					return;
				}

				p_out[0] = p_in.length();
				p_out[2] = asin(p_in[1] / p_out[0]);
				p_out[1] = atan2(p_in[0], p_in[2]);
			}

			template<typename Vector>
			static inline void cubeToSphere(Vector& p_in, Vector& p_out)
			{
				float xtemp = p_in[0];
				float ytemp = p_in[1];
				float ztemp = p_in[2];

				p_out[0] = xtemp * sqrt(1.0 - ytemp * ytemp * 0.5 - ztemp * ztemp * 0.5 + ytemp * ytemp * ztemp * ztemp / 3.0);
				p_out[1] = ytemp * sqrt(1.0 - ztemp * ztemp * 0.5 - xtemp * xtemp * 0.5 + xtemp * xtemp * ztemp * ztemp / 3.0);
				p_out[2] = ztemp * sqrt(1.0 - xtemp * xtemp * 0.5 - ytemp * ytemp * 0.5 + xtemp * xtemp * ytemp * ytemp / 3.0);
			}

			template<typename Vector>
			static inline void vectorPlanetOrientation(int p_orientation, Vector& p_in, Vector& p_out)
			{
				Vector res;

				if (0 == p_orientation) // front
				{
					p_out[0] = p_in[0];
					p_out[1] = p_in[1];
					p_out[2] = p_in[2];
				}
				else if (1 == p_orientation) // rear
				{
					p_out[0] = -p_in[0];
					p_out[1] = p_in[1];
					p_out[2] = -p_in[2];
				}
				else if (2 == p_orientation) // left
				{
					p_out[0] = -p_in[2];
					p_out[1] = p_in[1];
					p_out[2] = p_in[0];
				}
				else if (3 == p_orientation) // right
				{
					p_out[0] = p_in[2];
					p_out[1] = p_in[1];
					p_out[2] = -p_in[0];
				}
				else if (4 == p_orientation) // top
				{
					p_out[0] = p_in[0];
					p_out[1] = p_in[2];
					p_out[2] = -p_in[1];
				}
				else //if( 5 == p_orientation ) // bottom
				{
					p_out[0] = p_in[0];
					p_out[1] = -p_in[2];
					p_out[2] = p_in[1];
				}
			}
        }
    }
}