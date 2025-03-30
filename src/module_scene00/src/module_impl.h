
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

#include "module_root.h"

#include "entity.h"
#include "entitygraph.h"
#include "renderingqueue.h"

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

    void                            createEntities(const std::string p_appWindowsEntityName);

    void                            resource_system_events();
    void                            d3d11_system_events();


    //override
    void                            registerSubscriber(const Callback& p_callback);

private:

    static constexpr int                    timeSystemSlot{ 0 };
    static constexpr int                    d3d11SystemSlot{ 1 };
    static constexpr int                    resourceSystemSlot{ 2 };
    static constexpr int                    worldSystemSlot{ 3 };
    static constexpr int                    renderingQueueSystemSlot{ 4 };
    static constexpr int                    dataPrintSystemSlot{ 5 };

    bool                                    m_show_mouse_cursor{ true };
    bool                                    m_mouse_relative_mode{ false };

    mage::core::Entitygraph             m_entitygraph;

    mage::rendering::Queue*             m_windowRenderingQueue{ nullptr };
    mage::rendering::Queue*             m_bufferRenderingQueue{ nullptr };

    bool                                    m_quadEntity0_state_request{ true };
    bool                                    m_quadEntity0_state { false };

    bool                                    m_quadEntity1_state_request{ true };
    bool                                    m_quadEntity1_state{ false };

    bool                                    m_quadEntity2_state_request{ true };
    bool                                    m_quadEntity2_state{ false };

    std::default_random_engine              m_generator;
    std::uniform_real_distribution<double>  m_distribution;
};




