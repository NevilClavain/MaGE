
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

#include <unordered_map>
#include <random>

#include "entity.h"
#include "entitygraph.h"
#include "renderingqueue.h"


class ModuleImpl : public mage::SamplesBase
{
public:

    ModuleImpl() = default;
    ~ModuleImpl() = default;

    ModuleImpl(const ModuleImpl&) = delete;
    ModuleImpl(ModuleImpl&&) = delete;
    ModuleImpl& operator=(const ModuleImpl& t) = delete;


    std::string                     getModuleName() const;
    std::string                     getModuleDescr() const;



    void                            onKeyPress(long p_key);
    void                            onEndKeyPress(long p_key);
    void                            onKeyPulse(long p_key);
    void                            onChar(long p_char, long p_scan);
    void                            onMouseMove(long p_xm, long p_ym, long p_dx, long p_dy);
    void                            onMouseWheel(long p_delta);
    void                            onMouseLeftButtonDown(long p_xm, long p_ym);
    void                            onMouseLeftButtonUp(long p_xm, long p_ym);
    void                            onMouseRightButtonDown(long p_xm, long p_ym);
    void                            onMouseRightButtonUp(long p_xm, long p_ym);
    void                            onAppEvent(WPARAM p_wParam, LPARAM p_lParam);

    void                            init(const std::string p_appWindowsEntityName);
    void                            run(void);
    void                            close(void);

private:

    mage::core::Entity*                                         m_groundEntity{ nullptr };
    mage::core::Entity*                                         m_cloudsEntity{ nullptr };
    mage::core::Entity*                                         m_treeEntity{ nullptr };
    mage::core::Entity*                                         m_skydomeEntity{ nullptr };

    std::string                                                 m_currentCamera;

    static constexpr double                                     groundLevel{ 0 };

    static constexpr double                                     skydomeSkyfromspace_ESun{ 8.7 };
    static constexpr double                                     skydomeSkyfromatmo_ESun{ 70.0 };
    static constexpr double                                     skydomeGroundfromspace_ESun{ 24.0 };
    static constexpr double                                     skydomeGroundfromatmo_ESun{ 12.0 };

    static constexpr double                                     skydomeAtmoThickness{ 1600.0 };
    static constexpr double                                     skydomeOuterRadius{ 70000.0 };
    static constexpr double                                     skydomeInnerRadius{ skydomeOuterRadius - skydomeAtmoThickness };

    static constexpr double                                     skydomeWaveLength_x{ 0.650 };
    static constexpr double                                     skydomeWaveLength_y{ 0.570 };
    static constexpr double                                     skydomeWaveLength_z{ 0.475 };
    static constexpr double                                     skydomeKm{ 0.0010 };
    static constexpr double                                     skydomeKr{ 0.0033 };
    static constexpr double                                     skydomeScaleDepth{ 0.25 };


    void                            resource_system_events();
    void                            d3d11_system_events();
    
    void                            create_scenegraph(const std::string& p_mainWindowsEntityId);
    void                            create_textures_channel_rendergraph(const std::string& p_queueEntityId);
    void                            create_zdepth_channel_rendergraph(const std::string& p_queueEntityId);

};
