
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

#include "samplesbase.h"

namespace mage
{
    class SamplesOpenEnv : public SamplesBase
    {
    public:

        SamplesOpenEnv() = default;
        ~SamplesOpenEnv() = default;

        SamplesOpenEnv(const SamplesOpenEnv&) = delete;
        SamplesOpenEnv(SamplesOpenEnv&&) = delete;
        SamplesOpenEnv& operator=(const SamplesOpenEnv& t) = delete;

        void                            init(const std::string p_appWindowsEntityName);
        void                            run(void);

    protected:

        mage::core::Entity*             m_groundEntity{ nullptr };
        mage::core::Entity*             m_cloudsEntity{ nullptr };
        mage::core::Entity*             m_treeEntity{ nullptr };
        mage::core::Entity*             m_skydomeEntity{ nullptr };
        mage::core::Entity*             m_sphereEntity{ nullptr };
        mage::core::Entity*             m_wallEntity{ nullptr };

        //std::string                     m_currentCamera;

        static constexpr double         groundLevel{ -100 };

        static constexpr double         skydomeSkyfromspace_ESun{ 8.7 };
        static constexpr double         skydomeSkyfromatmo_ESun{ 70.0 };
        static constexpr double         skydomeGroundfromspace_ESun{ 24.0 };
        static constexpr double         skydomeGroundfromatmo_ESun{ 12.0 };

        static constexpr double         skydomeAtmoThickness{ 1600.0 };
        static constexpr double         skydomeOuterRadius{ 70000.0 };
        static constexpr double         skydomeInnerRadius{ skydomeOuterRadius - skydomeAtmoThickness };

        static constexpr double         skydomeWaveLength_x{ 0.650 };
        static constexpr double         skydomeWaveLength_y{ 0.570 };
        static constexpr double         skydomeWaveLength_z{ 0.475 };
        static constexpr double         skydomeKm{ 0.0010 };
        static constexpr double         skydomeKr{ 0.0033 };
        static constexpr double         skydomeScaleDepth{ 0.25 };

        mage::core::maths::Matrix       m_perpective_projection;
        mage::core::maths::Matrix       m_orthogonal_projection;


        void                            d3d11_system_events_openenv();

        void                            create_openenv_scenegraph(const std::string& p_parentEntityId);
        void                            create_openenv_rendergraph(const std::string& p_parentEntityId, int p_w_width, int p_w_height, float p_characteristics_v_width, float p_characteristics_v_height);
        
        void                            install_shadows_renderer_queues(int p_w_width, int p_w_height, float p_characteristics_v_width, float p_characteristics_v_height);

        void                            install_shadows_renderer_objects();
    };
}