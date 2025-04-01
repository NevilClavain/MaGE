
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
#include <string>

#include "module_root.h"

#include "entity.h"
#include "entitygraph.h"
#include "renderingqueue.h"

#include "animations.h"

namespace mage
{
    class SamplesBase : public mage::interfaces::ModuleRoot
    {
    public:
        SamplesBase();
        ~SamplesBase() = default;

        SamplesBase(const SamplesBase&) = delete;
        SamplesBase(SamplesBase&&) = delete;
        SamplesBase& operator=(const SamplesBase& t) = delete;


        mage::core::Entitygraph*        entitygraph();

        void                            init(const std::string p_appWindowsEntityName);
        void                            run(void);
        void                            close(void);

    protected:

        //override
        void                            registerSubscriber(const Callback& p_callback);


        void                            d3d11_system_events_base();

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

        mage::core::Entity*                                         m_loading_gear{ nullptr };
        mage::core::Entity*                                         m_logo{ nullptr };

    };
}


