
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

#include <array>
#include <memory>
#include <functional>

#include "exceptions.h"

//                        QuadTree                                                                                                              
//                                                                                                                                              
// +-------------------------------+------------------------------+           +-------------------------------+------------------------------+  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |              Index 0          |         Index 1              |           |             Index 5           |            Index 6           |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |         L0_UP_LEFT            |        L0_UP_RIGHT           |           |         L1_UP_LEFT            |         L1_UP_RIGHT          |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// +-------------------------------+------------------------------+           +-------------------------------+------------------------------+  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |              Index 3          |         Index 2              |           |              Index 4          |             Index 7          |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |       L0_DOWN_LEFT            |      L0_DOWN_RIGHT           |           |        L1_DOWN_LEFT           |         L1_DOWN_RIGHT        |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// +-------------------------------+------------------------------+           +-------------------------------+------------------------------+  
//                                                  ^                                      ^                                                    
//                                                  |                                      |                                                    
//                                                  |                                      |                                                    
//                                                  |                                      |                                                    
//                                                  |                                      |                                                    
//                                                  +--------  OctTree  -------------------+                                                    
//                                                                                                                                              
//                                                                                                                                              
//       
// 
// for each node :
//	up_neighbour, down_neighbour, left_neighbour, right_neighbour
// 
// default neigbours at creation (parent split) :
// 
// L0_UP_LEFT:
//	up_neighbour = null
//	down_neighbour = L0_DOWN_LEFT
//	left_neighbour = null
//	right_neighbour = L0_UP_RIGHT
//  top_neighbour = L1_UP_LEFT
//  bottom_neighbour = nullptr
// 
//
// L0_UP_RIGHT:
//	up_neighbour = null
//	down_neighbour = L0_DOWN_RIGHT
//	left_neighbour = L0_UP_LEFT
//	right_neighbour = null
//  top_neighbour = L1_UP_RIGHT
//  bottom_neighbour = nullptr
// 
//
// L0_DOWN_LEFT:
//	up_neighbour = L0_UP_LEFT
//	down_neighbour = null
//	left_neighbour = null
//	right_neighbour = L0_DOWN_RIGHT
//  top_neighbour = L1_DOWN_LEFT
//  bottom_neighbour = nullptr
// 
//
// L0_DOWN_RIGHT:
//	up_neighbour = L0_UP_RIGHT
//	down_neighbour = null
//	left_neighbour = L0_DOWN_LEFT
//	right_neighbour = null
//  top_neighbour = L1_DOWN_RIGHT
//  bottom_neighbour = nullptr


namespace mage
{
	namespace core
	{
		template <size_t ChildCount, typename NodeData>
		class XTreeNode
		{
		public:

			enum class Index
			{
				L0_UP_LEFT_INDEX = 0,
				L0_UP_RIGHT = 1,
				L0_DOWN_RIGHT_INDEX = 2,
				L0_DOWN_LEFT_INDEX = 3,

				L1_DOWN_LEFT_INDEX = 4,
				L1_UP_LEFT_INDEX = 5,
				L1_UP_RIGHT_INDEX = 6,
				L1_DOWN_RIGHT_INDEX = 7
			};

			enum class Neighbour
			{
				UP_NEIGHBOUR,
				DOWN_NEIGHBOUR,
				LEFT_NEIGHBOUR,
				RIGHT_NEIGHBOUR,
				TOP_NEIGHBOUR,    // used in octtree case
				BOTTOM_NEIGHBOUR // used in octtree case
			};

			XTreeNode() : m_depth(0) {}
			explicit XTreeNode(const NodeData& data, size_t depth = 0) : m_data(data), m_depth(depth) {}

			XTreeNode* getChild(size_t index) const
			{
				if (index < ChildCount && m_children.at(index))
				{
					return m_children.at(index).get();
				}
				return nullptr;
			}

			const NodeData& getData() const
			{
				return m_data;
			}

			void setData(const NodeData& data)
			{
				m_data = data;
			}

			size_t getDepth() const
			{
				return m_depth;
			}

			bool isLeaf() const
			{
				for (const auto& child : m_children)
				{
					if (child) return false;
				}
				return true;
			}

			void split()
			{
				for (int i = 0; i < ChildCount; i++)
				{
					XTreeNode* node{ create_child(i) };

					switch (i)
					{
						default:
							break;
					}
				}
			}

			void merge()
			{
				for (size_t i = 0; i < ChildCount; ++i)
				{
					m_children.at(i).reset();
				}
			}

			void traverse(const std::function<void(const NodeData&, size_t)>& func) const
			{
				func(m_data, m_depth);
				for (const auto& child : m_children)
				{
					if (child)
					{
						child->traverse(func);
					}
				}
			}

		private:

			XTreeNode* create_child(size_t index)
			{
				if (index < ChildCount)
				{
					if (m_children.at(index))
					{
						_EXCEPTION("children already exists")
					}
					m_children[index] = std::make_unique<XTreeNode>();
					m_children.at(index)->m_depth = m_depth + 1;

					return m_children.at(index).get();
				}
				return nullptr;
			}

			NodeData												m_data;
			std::array<std::unique_ptr<XTreeNode>, ChildCount>		m_children;
			size_t													m_depth{ 0 };

			//std::array<XTreeNode*, ?>						m_neighbours;
		};

		template <typename NodeData>
		using QuadTreeNode = XTreeNode<4, NodeData>;

		template <typename NodeData>
		using OctreeNode = XTreeNode<8, NodeData>;

	} // core
} // mage
