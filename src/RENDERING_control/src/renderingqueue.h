
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

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "tvector.h"
#include "matrix.h"
#include "renderstate.h"
#include "shader.h"
#include "texture.h"

namespace mage
{
	class RenderingQueueSystem;
	class D3D11System;

	namespace core
	{
		template<typename T>
		class Component;

		template<typename T>
		using ComponentList = std::vector<Component<T>*>;
	}

	namespace rendering
	{
		//fwd decl
		struct PrimitiveDrawing;	
		class RenderingQueueSystem;


		/// IN ENTITY
		struct DrawingControl
		{
		public:

			DrawingControl()
			{
				//world.identity();
			}

			~DrawingControl() = default;

			//core::maths::Matrix world;

			bool				ready{ false };

			bool                projected_z_neg{ false }; // for objects that takes their pos from projected position (some 2d sprites...)


			//shaders params mapping description
			// dataCloud variable id / shader argument section id in shader json
			std::vector<std::pair<std::string, std::string>> vshaders_map;
			std::vector<std::pair<std::string, std::string>> pshaders_map;


			std::string owner_entity_id; // to be completed by queue system

			bool				draw{ true };

		};

		/// 'DrawingControl' EQUIVALENT IN BUILT RENDERING QUEUE

		struct QueueDrawingControl
		{
			virtual ~QueueDrawingControl() = default;

			// transformations to apply;

			std::vector<const core::maths::Matrix*> worlds;

			bool* projected_z_neg{ nullptr };

			// shaders generic params to apply
			// dataCloud variable id/shader argument
			std::vector<std::pair<std::string, mage::Shader::GenericArgument>>	vshaders_map_cnx; // computed from vshaders_map and the queue current vshader
			std::vector<std::pair<std::string, mage::Shader::GenericArgument>>	pshaders_map_cnx; // computed from pshaders_map and the queue current pshader

			// shaders vector arrays to apply
			const std::vector<mage::Shader::VectorArrayArgument>* vshaders_vector_array{ nullptr };
			const std::vector<mage::Shader::VectorArrayArgument>* pshaders_vector_array{ nullptr };

			std::string owner_entity_id;

			bool* draw{ nullptr };
		};


		struct QueueTrianglesDrawingControl : public QueueDrawingControl
		{
		public:

			// meshe to set
			std::string						meshe_id;

			// textures stages to set

			std::unordered_map<size_t, std::string>	textures; // stage/texture id

			bool operator==(const QueueTrianglesDrawingControl& p_other) const
			{
				return meshe_id == p_other.meshe_id && textures == p_other.textures;
			}
		};

		struct QueueLinesDrawingControl : public QueueDrawingControl
		{
		public:			
			// meshe to set
			std::string						meshe_id;

			bool operator==(const QueueTrianglesDrawingControl& p_other) const
			{
				return meshe_id == p_other.meshe_id;
			}
		};

		struct Queue
		{
		public:

			enum class Purpose
			{
				UNDEFINED,
				SCREEN_RENDERING,
				BUFFER_RENDERING
			};

			enum class State
			{
				WAIT_INIT,
				READY,
				// useless (throw exception instead)
				//ERROR_ORPHAN
			};

			struct Text
			{
				std::string							text;
				std::string							font;
				mage::core::maths::RGBAColor		color{ 255, 255, 255, 255 };
				mage::core::maths::IntCoords2D		position;
				float								rotation_rad{ 0.0 };
			};

			/////////////////////////////////////////////////////////////////////////////////////////

			struct RenderStatePayload
			{
				// renderstates set
				std::vector<RenderState>								description;

				// key = triangleMeshe D3D11 id
				//std::unordered_map<std::string, TriangleMeshePayload>	trianglemeshes_list;

				// key = lineMeshe D3D11 id
				//std::unordered_map<std::string, LineMeshePayload>		linemeshes_list;

				// key = QueueTrianglesDrawingControl unique id
				std::unordered_map<std::string, QueueTrianglesDrawingControl>	triangles_dc_list;

				// key = QueueLinesDrawingControl unique id
				std::unordered_map<std::string, QueueLinesDrawingControl>		lines_dc_list;

				

			};

			struct ShadersPayload
			{ 
				std::vector<std::string> shaders_ids; // ALWAYS 2 entries for now (vertex shader, pixel shader)

				// key = renderstate set strings dump concatenation (RenderState::toString())
				std::unordered_map<std::string, RenderStatePayload> list;
			};

			struct RenderingOrderChannel
			{
				// key = shaders id concatenation
				std::unordered_map<std::string, ShadersPayload> list; 
			};

			using QueueNodes = std::map<int, RenderingOrderChannel>;  // RenderingOrderChannel are rendered following order given by int key

			////////////////////////////////////////////////////////////////////

			Queue(const std::string& p_name);
			Queue(const Queue& p_other);

			Queue& operator=(const Queue& p_other)
			{
				m_name = p_other.m_name;
				m_purpose = p_other.m_purpose;
				m_state = p_other.m_state;
				m_clear_target = p_other.m_clear_target;
				m_target_clear_color = p_other.m_target_clear_color;
				m_clear_target_depth = p_other.m_clear_target_depth;
				m_texts = p_other.m_texts;
				m_queueNodes = p_other.m_queueNodes;
				
				
				m_queueNodesA = p_other.m_queueNodesA;
				m_queueNodesB = p_other.m_queueNodesB;

				m_queueMutex.lock();
				m_queueNodesFront = p_other.m_queueNodesFront;
				m_queueNodesBack = p_other.m_queueNodesBack;
				m_queueMutex.unlock();

				m_mainView = p_other.m_mainView;
				m_secondaryView = p_other.m_secondaryView;
				m_targetStage = p_other.m_targetStage;

				m_targetTextureUID = p_other.m_targetTextureUID;

				return *this;
			}


			~Queue() = default;

			std::string					getName() const;
			Purpose						getPurpose() const;
			State						getState() const;

			void						enableTargetClearing(bool p_enable);
			void						setTargetClearColor(const core::maths::RGBAColor& p_color);

			void						enableTargetDepthClearing(bool p_enable);

			bool						getTargetClearing() const;
			core::maths::RGBAColor		getTargetClearColor() const;


			bool						getTargetDepthClearing() const;

			void						pushText(const Text& p_text);
			std::vector<Text>			getTexts() const;
			void						setTexts(const std::vector<Text>& p_texts);
			void						clearTexts();

			QueueNodes					getQueueNodes() const;
			void						setQueueNodes(const QueueNodes& p_nodes);

			QueueNodes*					queueNodesFrontAccess();
			QueueNodes*					queueNodesBackAccess();
			void						switchQueues();

			void						setMainView(const std::string& p_entityId);
			std::string					getMainView() const;

			void						setSecondaryView(const std::string& p_entityId);
			std::string					getSecondaryView() const;

			
			std::string					getTargetTextureUID() const;

			void						setTargetStage(size_t p_stage);
			size_t						getTargetStage() const;

			void						resetStates();

			void						setState(State p_newstate);

			void						setScreenRenderingPurpose();
			void						setBufferRenderingPurpose(mage::Texture& p_target_texture);

								
		private:
	
			std::string						m_name;
			Purpose							m_purpose{ Purpose::UNDEFINED };
			State							m_state{ State::WAIT_INIT };


			bool							m_clear_target{ false };
			core::maths::RGBAColor			m_target_clear_color;

			bool							m_clear_target_depth{ false };

			std::vector<Text>				m_texts;

			QueueNodes						m_queueNodes;

			QueueNodes						m_queueNodesA;
			QueueNodes						m_queueNodesB;

			QueueNodes*						m_queueNodesFront	{ &m_queueNodesA };
			QueueNodes*						m_queueNodesBack	{ &m_queueNodesB };

			mutable std::mutex				m_queueMutex;

			std::string						m_mainView; // entity name
			std::string						m_secondaryView; // entity name


			size_t							m_targetStage{ 0 };

			std::string						m_targetTextureUID; // for BUFFER_RENDERING			

			// IF NEW MEMBERS HERE :
			// UPDATE COPY CTOR AND OPERATOR !!!!!!

		};
	}
}
