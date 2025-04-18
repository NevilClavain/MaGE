
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
#include <random>

#include "samplesbase.h"

#include "entity.h"
#include "entitygraph.h"
#include "renderingqueue.h"
#include "texture.h"

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

    void                            resource_system_events();
    void                            d3d11_system_events();


    void                            manage_sprite_trajectory(mage::core::Entity* p_entity);


private:

    std::default_random_engine*             m_generator;
    
    std::uniform_real_distribution<double>* m_speed_distribution;
    std::uniform_int_distribution<int>*     m_speed_sign_distribution;

    std::uniform_real_distribution<double>* m_rotation_speed_distribution;
    std::uniform_int_distribution<int>*     m_rotation_speed_sign_distribution;


    std::vector<mage::core::Entity*>        m_sprites;

    std::string                             m_currentCamera;
};




