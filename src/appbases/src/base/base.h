
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

#include <json_struct/json_struct.h>

#include "module_root.h"

#include "entity.h"
#include "entitygraph.h"
#include "renderingqueue.h"

#include "animations.h"

namespace mage
{
    namespace json
    {
        struct DataCloudVar
        {
            std::string     name;
            std::string     type;

            long            integer;
            double          vec[4];
            std::string     str;
            
            JS_OBJ(name, type, integer, vec, str);
        };
    }

    class Base : public mage::interfaces::ModuleRoot
    {
    public:
        Base();
        ~Base() = default;

        Base(const Base&) = delete;
        Base(Base&&) = delete;
        Base& operator=(const Base& t) = delete;


        mage::core::Entitygraph*        entitygraph();

        void                            init(const std::string p_appWindowsEntityName);
        void                            run(void);
        void                            close(void);

    protected:

        //override
        void                            registerSubscriber(const Callback& p_callback);
        void                            d3d11_system_events_base();

        static constexpr int            timeSystemSlot{ 0 };
        static constexpr int            d3d11SystemSlot{ 1 };
        static constexpr int            resourceSystemSlot{ 2 };
        static constexpr int            worldSystemSlot{ 3 };
        static constexpr int            renderingQueueSystemSlot{ 4 };
        static constexpr int            dataPrintSystemSlot{ 5 };
        static constexpr int            animationsSystemSlot{ 6 };
        static constexpr int            sceneStreamSystemSlot{ 7 };

        bool                            m_show_mouse_cursor{ false };
        bool                            m_mouse_relative_mode{ true };

        std::string                     m_appWindowsEntityName;

        int                             m_w_width;
        int                             m_w_height;

        float                           m_characteristics_v_width;
        float                           m_characteristics_v_height;

        mage::core::Entitygraph         m_entitygraph;

        mage::rendering::Queue*         m_windowRenderingQueue{ nullptr };

        mage::core::Entity*             m_loading_gear{ nullptr };
        mage::core::Entity*             m_logo{ nullptr };

    };
}


