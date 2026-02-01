
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

#include "maths_helpers.h"
#include "openenv.h"

#include <unordered_map>
#include <random>

#include "entity.h"
#include "entitygraph.h"
#include "renderingqueue.h"

#include "animations.h"

class ModuleImpl : public mage::OpenEnv
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

    mage::core::Entity*                                         m_raptorEntity{ nullptr };

    std::unordered_map<std::string, mage::AnimationKeys>        m_raptor_animations;

    std::default_random_engine                                  m_random_engine;
    std::uniform_int_distribution<int>*                         m_distribution;


    double                                                      m_light_theta { 0 }; // azimutal
    double                                                      m_light_phi   { - mage::core::maths::pi / 3 }; // elevation
    double                                                      m_light_ray   { 1000.0 };


    bool                                                        m_left_ctrl{ false };


    bool                                                        m_appReady{ false };

    void                            resource_system_events();
    void                            d3d11_system_events();
    void                            animation_system_events();

    void                            choose_animation();
    
    
    void                            complete_scenegraph(const std::string& p_mainWindowsEntityId);
    
    void                            complete_install_shadows_renderer_objects();
};
