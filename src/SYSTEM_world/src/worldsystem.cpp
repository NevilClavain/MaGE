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

#include <chrono>
#include <string>

#include "worldsystem.h"
#include "entity.h"
#include "entitygraph.h"
#include "aspects.h"
#include "ecshelpers.h"
#include "worldposition.h"
#include "animatorfunc.h"
#include "renderingqueue.h"
#include "matrixchain.h"
#include "datacloud.h"
#include "matrix.h"

using namespace mage;
using namespace mage::core;

WorldSystem::WorldSystem(Entitygraph& p_entitygraph) : System(p_entitygraph)
{
	const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
	dataCloud->registerData<std::string>("mage.timings.worldsystem");
	dataCloud->registerData<std::string>("mage.timings.worldsystem.part1");
	dataCloud->registerData<std::string>("mage.timings.worldsystem.part2");

	// Register callback for entitygraph events
	m_entitygraph.registerSubscriber([this](core::EntitygraphEvents p_event, const core::Entity& p_entity)
	{
		switch (p_event)
		{
			case core::EntitygraphEvents::ENTITYGRAPHNODE_ADDED:
			{
				// push it to the queue to be processed later - in next run() call, because entity was just created so no any aspects added yet at this moment

				m_newly_added_entities.push(const_cast<core::Entity*>(&p_entity));
			}
			break;

			case core::EntitygraphEvents::ENTITYGRAPHNODE_REMOVED:
			{
				if (m_entities_to_compute_distance.count(const_cast<core::Entity*>(&p_entity)))
				{
					m_entities_to_compute_distance.erase(const_cast<core::Entity*>(&p_entity));
				}

				if (m_entities_to_compute_2d_pos.count(const_cast<core::Entity*>(&p_entity)))
				{
					m_entities_to_compute_2d_pos.erase(const_cast<core::Entity*>(&p_entity));
				}

				if (m_entities_to_compute.count(const_cast<core::Entity*>(&p_entity)))
				{
					m_entities_to_compute.erase(const_cast<core::Entity*>(&p_entity));
				}
			}
			break;
		}
	});
}

void WorldSystem::extractProjAndViewFromRenderingQueue(const std::string& p_current_view_entity_id, mage::core::maths::Matrix& p_current_view, mage::core::maths::Matrix& p_current_proj)
{	
	if (p_current_view_entity_id != "")
	{
		auto& viewode{ m_entitygraph.node(p_current_view_entity_id) };
		const auto view_entity{ viewode.data() };

		// extract cam aspect
		const auto& cam_aspect{ view_entity->aspectAccess(cameraAspect::id) };
		const auto& cam_projs_list{ cam_aspect.getComponentsByType<maths::Matrix>() };

		if (0 == cam_projs_list.size())
		{
			_EXCEPTION("entity view aspect : missing projection definition " + view_entity->getId());
		}
		else
		{
			p_current_proj = cam_projs_list.at(0)->getPurpose();			
		}

		// extract world aspect

		const auto& world_aspect{ view_entity->aspectAccess(worldAspect::id) };
		const auto& worldpositions_list{ world_aspect.getComponentsByType<transform::WorldPosition>() };

		if (0 == worldpositions_list.size())
		{
			_EXCEPTION("entity world aspect : missing world position " + view_entity->getId());
		}
		else
		{
			auto& entity_worldposition{ worldpositions_list.at(0)->getPurpose() };
			const auto current_cam = entity_worldposition.global_pos;

			p_current_view = current_cam;
			p_current_view.inverse();
		}
	}
}

void WorldSystem::run()
{
	const auto start_time_main{ std::chrono::high_resolution_clock::now() };

	const auto dataCloud{ mage::rendering::Datacloud::getInstance() };

	//////////////////////////////////////////////////////////
	/// I : Process newly added entities from FIFO queue
	//////////////////////////////////////////////////////////

	while (!m_newly_added_entities.empty())
	{
		core::Entity* newly_added_entity{ m_newly_added_entities.front() };
		m_newly_added_entities.pop();

		// Process the newly added entity
		// Entity pointer is examined and can be used for initialization, registration, etc.
		if (newly_added_entity)
		{
			// specific logic to process the newly added entity

			if (newly_added_entity->hasAspect(core::worldAspect::id))
			{
				const auto& world_aspect{ newly_added_entity->aspectAccess(worldAspect::id) };

				/////////////// manage 2D pos and distance computing

				auto distancetocam_components_list{ world_aspect.getComponentsByType<std::pair<mage::rendering::Queue*, double>>() };
				if (distancetocam_components_list.size())
				{
					// add to related list
					m_entities_to_compute_distance.insert(newly_added_entity);
				}

				auto screenposition_components_list{ world_aspect.getComponentsByType<std::pair<mage::rendering::Queue*, core::maths::Real3Vector>>() };
				if (screenposition_components_list.size())
				{
					// add to related list
					m_entities_to_compute_2d_pos.insert(newly_added_entity);
				}

				/////////////// manage frozen object

				const bool frozen_tag{ mage::helpers::checkTag(newly_added_entity, "#frozen") };

				if (frozen_tag)
				{
					// TO BE CONTINUED

					// compute transformation only once
				}
				else
				{
					if (newly_added_entity->hasAspect(worldAspect::id))
					{
						const mage::core::tagsAspect::GraphDomain gd{ mage::helpers::getEntityGraphdomain(newly_added_entity) };

						if (mage::core::tagsAspect::GraphDomain::SCENEGRAPH == gd)
						{
							m_entities_to_compute.insert(newly_added_entity);
						}
						else
						{
							// ??
							// compute transformation only once
						}
					}					
				}

				///////////////////////////////////////////////////////////////
			}
		}
	}

	//////////////////////////////////////////////////////////
	/// II : compute transformations
	//////////////////////////////////////////////////////////

	const auto start_time_part1{ std::chrono::high_resolution_clock::now() };

	const auto forEachWorldAspect
	{
		[&](Entity* p_entity, const ComponentContainer& p_world_components)
		{
			compute_entity(p_entity, p_world_components);
		}
	};

	mage::helpers::extractAspectsTopDown<mage::core::worldAspect>(m_entitygraph, forEachWorldAspect);

	const auto end_time_part1{ std::chrono::high_resolution_clock::now() };
	const auto duration_part1{ std::chrono::duration_cast<std::chrono::milliseconds>(end_time_part1 - start_time_part1) };

	dataCloud->updateDataValue<std::string>("mage.timings.worldsystem.part1", std::to_string(duration_part1.count()) + " ms");

	//////////////////////////////////////////////////////////
	/// III : compute 2D pos and distance to cam (for entity that requires it)
	//////////////////////////////////////////////////////////

	// rebuid hierarchical structure to be browsed recursively
	
	const auto start_time_part2{ std::chrono::high_resolution_clock::now() };

	for (auto curr_entity : m_entities_to_compute_distance)
	{
		const auto& world_aspect{ curr_entity->aspectAccess(worldAspect::id) };

		auto distancetocam_components_list{ world_aspect.getComponentsByType<std::pair<mage::rendering::Queue*, double>>() };
		auto& distancetocamera_component{ distancetocam_components_list.at(0)->getPurpose().second };

		maths::Matrix current_view;
		maths::Matrix current_proj;

		auto& renderingQueue{ distancetocam_components_list.at(0)->getPurpose().first };

		const std::string current_view_entity_id{ renderingQueue->getMainView() };
		extractProjAndViewFromRenderingQueue(current_view_entity_id, current_view, current_proj);

		const auto& worldpositions_list{ world_aspect.getComponentsByType<transform::WorldPosition>() };

		if (0 == worldpositions_list.size())
		{
			_EXCEPTION("entity world aspect : missing world position " + curr_entity->getId());
		}
		else
		{
			auto& entity_worldposition{ worldpositions_list.at(0)->getPurpose() };
			maths::Matrix entity_world = entity_worldposition.global_pos;

			maths::Matrix inv;
			inv.identity();
			inv(2, 2) = -1.0;
			const auto final_view{ current_view * inv };

			transform::MatrixChain chain;
			chain.pushMatrix(final_view);
			chain.pushMatrix(entity_world);
			chain.buildResult();
			auto final_mat{ chain.getResultTransform() };

			core::maths::Real4Vector point(0, 0, 0, 1);
			core::maths::Real4Vector res_point;

			final_mat.transform(&point, &res_point);

			const double distance_to_cam{ res_point.length() };

			distancetocamera_component = distance_to_cam;
		}
	}

	for (auto curr_entity : m_entities_to_compute_2d_pos)
	{
		const auto& world_aspect{ curr_entity->aspectAccess(worldAspect::id) };

		auto screenposition_components_list{ world_aspect.getComponentsByType<std::pair<mage::rendering::Queue*, core::maths::Real3Vector>>() };

		auto& screenposition_component{ screenposition_components_list.at(0)->getPurpose().second };


		maths::Matrix current_view;
		maths::Matrix current_proj;

		auto& renderingQueue{ screenposition_components_list.at(0)->getPurpose().first };

		const std::string current_view_entity_id{ renderingQueue->getMainView() };
		extractProjAndViewFromRenderingQueue(current_view_entity_id, current_view, current_proj);

		const auto& worldpositions_list{ world_aspect.getComponentsByType<transform::WorldPosition>() };

		if (0 == worldpositions_list.size())
		{
			_EXCEPTION("entity world aspect : missing world position " + curr_entity->getId());
		}
		else
		{
			auto& entity_worldposition{ worldpositions_list.at(0)->getPurpose() };
			maths::Matrix entity_world = entity_worldposition.global_pos;

			maths::Matrix inv;
			inv.identity();
			inv(2, 2) = -1.0;
			const auto final_view{ current_view * inv };

			transform::MatrixChain chain;

			chain.pushMatrix(current_proj);
			chain.pushMatrix(final_view);
			chain.pushMatrix(entity_world);
			chain.buildResult();
			auto final_mat{ chain.getResultTransform() };

			core::maths::Real4Vector point(0, 0, 0, 1);
			core::maths::Real4Vector res_point;

			final_mat.transform(&point, &res_point);

			const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
			const auto viewport{ dataCloud->readDataValue<maths::FloatCoords2D>("mage.infos.viewport") };


			const double posx{ static_cast<double>(res_point[0] / (res_point[2] + 1.0)) * 0.5 * viewport[0] };
			const double posy{ static_cast<double>(res_point[1] / (res_point[2] + 1.0)) * 0.5 * viewport[1] };

			core::maths::Real3Vector projected_pos(posx, posy, res_point[2]);
			screenposition_component = projected_pos;
		}

	}

	const auto end_time_part2{ std::chrono::high_resolution_clock::now() };
	const auto duration_part2{ std::chrono::duration_cast<std::chrono::milliseconds>(end_time_part2 - start_time_part2) };

	dataCloud->updateDataValue<std::string>("mage.timings.worldsystem.part2", std::to_string(duration_part2.count()) + " ms");



	const auto end_time_main{ std::chrono::high_resolution_clock::now() };
	const auto duration_main{ std::chrono::duration_cast<std::chrono::milliseconds>(end_time_main - start_time_main) };
	
	dataCloud->updateDataValue<std::string>("mage.timings.worldsystem", std::to_string(duration_main.count()) + " ms");
}

void WorldSystem::compute_entity(core::Entity* p_entity, const ComponentContainer& p_world_components)
{
	///// compute matrix hierarchy

	const auto& entity_worldposition_list{ p_world_components.getComponentsByType<transform::WorldPosition>() };
	if (0 == entity_worldposition_list.size())
	{
		//_EXCEPTION("Entity world aspect : missing world position " + p_entity->getId());

		// just ignore
		return;
	}

	auto& entity_worldposition{ entity_worldposition_list.at(0)->getPurpose() };


	// /!\ local_pos CLEARED HERE 
	entity_worldposition.local_pos.identity();

	// get parent entity if exists
	const auto parent_entity{ p_entity->getParent() };

	if (parent_entity && parent_entity->hasAspect(worldAspect::id))
	{
		const auto& parent_worldaspect{ parent_entity->aspectAccess(worldAspect::id) };
		const auto& parententity_worldpositions_list{ parent_worldaspect.getComponentsByType<transform::WorldPosition>() };

		if (0 == parententity_worldpositions_list.size())
		{
			// _EXCEPTION("Parent entity world aspect : missing world position " + parent_entity->getId());
			// just ignore
		}
		else
		{
			auto& parententity_worldposition{ parententity_worldpositions_list.at(0)->getPurpose() };

			///// compute animators -> result stored in local pos

			auto& entity_animators_list{ p_world_components.getComponentsByType<transform::Animator>() };
			if (entity_animators_list.size() > 0)
			{
				if (p_entity->hasAspect(core::timeAspect::id))
				{
					const auto& time_aspect{ p_entity->aspectAccess(core::timeAspect::id) };

					for (const auto& animator_comp : entity_animators_list)
					{
						const auto& animator{ animator_comp->getPurpose() };
						animator.func(p_world_components, time_aspect, parententity_worldposition, animator.component_keys);
					}
				}
				else
				{
					_EXCEPTION("animator requires a time aspect")
				}
			}

			///////////////////////

			switch (entity_worldposition.composition_operation)
			{
			case transform::WorldPosition::TransformationComposition::TRANSFORMATION_RELATIVE_FROM_PARENT:

				entity_worldposition.global_pos = entity_worldposition.local_pos * parententity_worldposition.global_pos;
				break;

			case transform::WorldPosition::TransformationComposition::TRANSFORMATION_ABSOLUTE:

				entity_worldposition.global_pos = entity_worldposition.local_pos;
				break;

			case transform::WorldPosition::TransformationComposition::TRANSFORMATION_PARENT_PROJECTEDPOS:
			{
				auto screenposition_components_list{ parent_worldaspect.getComponentsByType<std::pair<mage::rendering::Queue*, core::maths::Real3Vector>>() };
				if (screenposition_components_list.size())
				{
					auto screenposition{ screenposition_components_list.at(0)->getPurpose().second };

					auto updated_local_pos{ entity_worldposition.local_pos };

					updated_local_pos(3, 0) += screenposition[0];
					updated_local_pos(3, 1) += screenposition[1];

					entity_worldposition.projected_z_neg = (screenposition[2] < 0);
					entity_worldposition.global_pos = updated_local_pos;

					if (p_entity->hasAspect(core::renderingAspect::id))
					{
						const auto& entity_renderingaspect{ p_entity->aspectAccess(core::renderingAspect::id) };

						auto& entity_dc_list{ entity_renderingaspect.getComponentsByType<rendering::DrawingControl>() };
						if (entity_dc_list.size() > 0)
						{
							entity_dc_list.at(0)->getPurpose().projected_z_neg = (screenposition[2] < 0);
						}
					}
				}
				else
				{
					_EXCEPTION("TRANSFORMATION_PARENT_PROJECTEDPOS mode require 2D projected pos from parent")
				}
			}
			break;
			}
		}
	}
	else
	{
		///// compute animators -> result stored in local pos

		auto& entity_animators_list{ p_world_components.getComponentsByType<transform::Animator>() };
		if (entity_animators_list.size() > 0)
		{
			if (p_entity->hasAspect(core::timeAspect::id))
			{
				const auto& time_aspect{ p_entity->aspectAccess(core::timeAspect::id) };

				for (const auto& animator_comp : entity_animators_list)
				{
					const auto& animator{ animator_comp->getPurpose() };

					// no parent -> give WorldPosition with identity
					transform::WorldPosition fake_parent_pos;
					fake_parent_pos.global_pos.identity();
					fake_parent_pos.local_pos.identity();

					animator.func(p_world_components, time_aspect, fake_parent_pos, animator.component_keys);
				}
			}
			else
			{
				_EXCEPTION("animator requires a time aspect")
			}
		}

		///////////////////////

		entity_worldposition.global_pos = entity_worldposition.local_pos;
	}

}