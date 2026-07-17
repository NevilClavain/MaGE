
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

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "system.h"
#include "logsink.h"
#include "logconf.h"
#include "logging.h"
#include "eventsource.h"

namespace mage
{
    namespace core { class Entity; }
    namespace core { class Entitygraph; }
    namespace core { class ComponentContainer; }
    namespace rendering { struct Queue; struct QueueDrawingControl; }

    enum class RenderingQueueSystemEvent
    {
        MAINVIEW_QUEUE_UPDATED,
        SECONDARYVIEW_QUEUE_UPDATED,
        RENDERINGQUEUE_STATE_READY
    };

    struct ViewGroup
    {
        ViewGroup() = default;

        std::string                         main_view;
        std::string                         secondary_view;

        std::unordered_set<std::string>     queues_id_list;
    };

    class RenderingQueueSystem : public core::System, public mage::property::EventSource<RenderingQueueSystemEvent, const std::string&, const mage::rendering::Queue&>
    {
    public:

        RenderingQueueSystem() = delete;
        RenderingQueueSystem(core::Entitygraph& p_entitygraph);
        ~RenderingQueueSystem() = default;

        void        run();
        void        requestRenderingqueueLogging(const std::string& p_entityid);

        void        createViewGroup(const std::string& p_viewGroupId);

        void        setViewGroupMainView(const std::string& p_viewGroupId, const std::string& p_mainview);
        void        setViewGroupSecondaryView(const std::string& p_viewGroupId, const std::string& p_secondaryview);

        void        addQueuesToViewGroup(const std::string& p_viewGroupId, const std::unordered_set<std::string>& p_queues_id_list);

        std::pair<std::string, std::string> getViewGroupCurrentViews(const std::string& p_viewGroupId) const;

        static void checkEntityInsertion(
            mage::core::Entity* p_entity,
            const mage::core::ComponentContainer& p_resourceAspect,
            const mage::core::ComponentContainer& p_renderingAspect,
            mage::rendering::Queue& p_renderingQueue);

        static void removeFromRenderingQueue(
            const std::string& p_entity_id,
            mage::rendering::Queue& p_renderingQueue);

    private:

        mutable mage::core::logger::Sink                    m_localLogger;
        std::unordered_set<std::string>                     m_queuesToLog;

        std::unordered_map<std::string, ViewGroup>          m_cameraViewGroups;

        int                                                 m_streamersystem_slot;
        std::once_flag                                      m_initialization_once_flag;

        void manageRenderingQueue();
        void handleRenderingQueuesState(core::Entity* p_entity, rendering::Queue& p_renderingQueue);

        void logRenderingqueue(const std::string& p_entity_id, mage::rendering::Queue& p_renderingQueue) const;

        static void pushWorldOutputToQueueDrawingControl(mage::core::Entity* p_entity, rendering::QueueDrawingControl& p_outqtdc);
    };
}