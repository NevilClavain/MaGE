
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
#include <vector>
#include <utility>

#include "maths_helpers.h"

#include "base.h"

namespace mage
{
    class OpenEnv : public Base
    {
    public:

        OpenEnv() = default;
        ~OpenEnv() = default;

        OpenEnv(const OpenEnv&) = delete;
        OpenEnv(OpenEnv&&) = delete;
        OpenEnv& operator=(const OpenEnv& t) = delete;

        void                            init(const std::string p_appWindowsEntityName);
        void                            run(void);

    protected:

        mage::core::Entity*                                             m_groundEntity{ nullptr };
        mage::core::Entity*                                             m_cloudsEntity{ nullptr };
        mage::core::Entity*                                             m_treeEntity{ nullptr };
        mage::core::Entity*                                             m_skydomeEntity{ nullptr };
        mage::core::Entity*                                             m_sphereEntity{ nullptr };
        mage::core::Entity*                                             m_wallEntity{ nullptr };

       
        std::vector<std::pair<std::string, core::maths::Real3Vector>>   m_shadowmap_joints_list;

        mage::core::maths::Matrix                                       m_perpective_projection;
        mage::core::maths::Matrix                                       m_orthogonal_projection;






        static constexpr double         skydomeWaveLength_x{ 0.650 };
        static constexpr double         skydomeWaveLength_y{ 0.570 };
        static constexpr double         skydomeWaveLength_z{ 0.475 };
        static constexpr double         skydomeKm{ 0.0010 };
        static constexpr double         skydomeKr{ 0.0033 };
        static constexpr double         skydomeScaleDepth{ 0.25 };



        void                            d3d11_system_events_openenv();

        void                            create_openenv_scenegraph(const std::string& p_parentEntityId);
        void                            create_openenv_rendergraph(const std::string& p_parentEntityId, int p_w_width, int p_w_height, float p_characteristics_v_width, float p_characteristics_v_height);

        void                            enable_shadows();

    };
}