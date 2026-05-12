
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

namespace mage
{
	namespace core
	{
		// fwd decl
		class Entitygraph;

		class System
		{
		public:

			static constexpr int            timeSystemSlot{ 0 };
			static constexpr int            d3d11SystemSlot{ 1 };
			static constexpr int            resourceSystemSlot{ 2 };
			static constexpr int            worldSystemSlot{ 3 };
			static constexpr int            renderingQueueSystemSlot{ 4 };
			static constexpr int            dataPrintSystemSlot{ 5 };
			static constexpr int            animationsSystemSlot{ 6 };
			static constexpr int            sceneStreamSystemSlot{ 7 };

			System(Entitygraph& p_entitygraph);
			~System() = default;

			virtual void run() = 0;
		
		protected:
			Entitygraph&	m_entitygraph;
		};
	}
}
