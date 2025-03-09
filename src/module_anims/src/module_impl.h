
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

#include <unordered_map>
#include <random>

#include "module_root.h"

#include "entity.h"
#include "entitygraph.h"
#include "renderingqueue.h"

#include "animations.h"

class ModuleImpl : public mage::interfaces::ModuleRoot
{
public:
    ModuleImpl();
    ~ModuleImpl() = default;

    ModuleImpl(const ModuleImpl&) = delete;
    ModuleImpl(ModuleImpl&&) = delete;
    ModuleImpl& operator=(const ModuleImpl& t) = delete;


    std::string                     getModuleName() const;
    std::string                     getModuleDescr() const;

    mage::core::Entitygraph*    entitygraph();

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

    static constexpr int                                        timeSystemSlot{ 0 };
    static constexpr int                                        d3d11SystemSlot{ 1 };
    static constexpr int                                        resourceSystemSlot{ 2 };
    static constexpr int                                        worldSystemSlot{ 3 };
    static constexpr int                                        renderingQueueSystemSlot{ 4 };
    static constexpr int                                        dataPrintSystemSlot{ 5 };
    static constexpr int                                        animationsSystemSlot{ 6 };

    bool                                                        m_show_mouse_cursor{ false };
    bool                                                        m_mouse_relative_mode{ true };

    std::string                                                 m_appWindowsEntityName;

    mage::core::Entitygraph                                     m_entitygraph;

    mage::rendering::Queue*                                     m_windowRenderingQueue{ nullptr };

    mage::core::Entity*                                         m_groundEntity{ nullptr };
    mage::core::Entity*                                         m_cloudsEntity{ nullptr };
    mage::core::Entity*                                         m_treeEntity{ nullptr };
    mage::core::Entity*                                         m_raptorEntity{ nullptr };
    mage::core::Entity*                                         m_skydomeEntity{ nullptr };

    std::unordered_map<std::string, mage::AnimationKeys>        m_raptor_animations;

    std::default_random_engine                                  m_random_engine;
    std::uniform_int_distribution<int>*                         m_distribution;

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
    void                            animation_system_events();

    //override
    void                            registerSubscriber(const Callback& p_callback);


    void                            choose_animation();
    
    void                            create_scenegraph(const std::string& p_mainWindowsEntityId);
    void                            create_textures_channel_rendergraph(const std::string& p_queueEntityId);
    void                            create_zdepth_channel_rendergraph(const std::string& p_queueEntityId);

};
