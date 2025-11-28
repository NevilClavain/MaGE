
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
#include <vector>
#include <memory>
#include <functional>
#include <cmath>

#include "exceptions.h"

//                        QuadTree                                                                                                              
//                                                                                                                                              
// +-------------------------------+------------------------------+           +-------------------------------+------------------------------+  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |                               |                              |           |                               |                              |  
// |              Index 0          |         Index 1              |           |             Index 4           |            Index 5           |  
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
// |              Index 3          |         Index 2              |           |              Index 7          |             Index 6          |  
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
//	up_neighbour = nullptr
//	down_neighbour = L0_DOWN_LEFT
//	left_neighbour = nullptr
//	right_neighbour = L0_UP_RIGHT
//  top_neighbour = L1_UP_LEFT
//  bottom_neighbour = nullptr
// 
//
// L0_UP_RIGHT:
//	up_neighbour = nullptr
//	down_neighbour = L0_DOWN_RIGHT
//	left_neighbour = L0_UP_LEFT
//	right_neighbour = nullptr
//  top_neighbour = L1_UP_RIGHT
//  bottom_neighbour = nullptr
// 
//
// L0_DOWN_RIGHT:
//	up_neighbour = L0_UP_RIGHT
//	down_neighbour = nullptr
//	left_neighbour = L0_DOWN_LEFT
//	right_neighbour = nullptr
//  top_neighbour = L1_DOWN_RIGHT
//  bottom_neighbour = nullptr
// 
// 
// L0_DOWN_LEFT:
//	up_neighbour = L0_UP_LEFT
//	down_neighbour = nullptr
//	left_neighbour = nullptr
//	right_neighbour = L0_DOWN_RIGHT
//  top_neighbour = L1_DOWN_LEFT
//  bottom_neighbour = nullptr


namespace mage
{
	namespace core
	{
		template <size_t Dim, typename NodeData>
		class XTreeNode
		{
		public:

			static constexpr unsigned int L0_UP_LEFT_INDEX		{ 0 };
			static constexpr unsigned int L0_UP_RIGHT_INDEX		{ 1 };
			static constexpr unsigned int L0_DOWN_RIGHT_INDEX	{ 2 };
			static constexpr unsigned int L0_DOWN_LEFT_INDEX	{ 3 };

			static constexpr unsigned int L1_UP_LEFT_INDEX		{ 4 };
			static constexpr unsigned int L1_UP_RIGHT_INDEX		{ 5 };
			static constexpr unsigned int L1_DOWN_RIGHT_INDEX	{ 6 };
			static constexpr unsigned int L1_DOWN_LEFT_INDEX	{ 7 };
			
					
			static constexpr unsigned int UP_NEIGHBOUR			{ 0 };
			static constexpr unsigned int DOWN_NEIGHBOUR		{ 1 };
			static constexpr unsigned int LEFT_NEIGHBOUR		{ 2 };
			static constexpr unsigned int RIGHT_NEIGHBOUR		{ 3 };
			static constexpr unsigned int TOP_NEIGHBOUR			{ 4 };			// used in octtree case
			static constexpr unsigned int BOTTOM_NEIGHBOUR		{ 5 };			// used in octtree case



			XTreeNode()
			{
				init_neighbours();
			}
	
			
			explicit XTreeNode(const NodeData& data) : m_data(data)
			{
				init_neighbours();
			}
			

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

			bool isRoot() const
			{
				if (!m_parent)
				{
					return true;
				}
				return false;
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
					create_child(i);
				}

				for (int i = 0; i < ChildCount; i++)
				{
					const auto child { m_children.at(i).get()};

					// compute default neighbours
					switch (i)
					{
						case L0_UP_LEFT_INDEX:

							child->m_neighbours.at(UP_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(DOWN_NEIGHBOUR) = m_children.at(L0_DOWN_LEFT_INDEX).get();
							child->m_neighbours.at(LEFT_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(RIGHT_NEIGHBOUR) = m_children.at(L0_UP_RIGHT_INDEX).get();

							if constexpr (3 == Dim) // octtree
							{
								child->m_neighbours.at(TOP_NEIGHBOUR) = m_children.at(L1_UP_LEFT_INDEX).get();
								child->m_neighbours.at(BOTTOM_NEIGHBOUR) = nullptr;
							}
							break;

						case L0_UP_RIGHT_INDEX:

							child->m_neighbours.at(UP_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(DOWN_NEIGHBOUR) = m_children.at(L0_DOWN_RIGHT_INDEX).get();
							child->m_neighbours.at(LEFT_NEIGHBOUR) = m_children.at(L0_UP_LEFT_INDEX).get();
							child->m_neighbours.at(RIGHT_NEIGHBOUR) = nullptr;

							if constexpr (3 == Dim) // octtree
							{
								child->m_neighbours.at(TOP_NEIGHBOUR) = m_children.at(L1_UP_RIGHT_INDEX).get();
								child->m_neighbours.at(BOTTOM_NEIGHBOUR) = nullptr;
							}
							break;

						case L0_DOWN_RIGHT_INDEX:

							child->m_neighbours.at(UP_NEIGHBOUR) = m_children.at(L0_UP_RIGHT_INDEX).get();
							child->m_neighbours.at(DOWN_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(LEFT_NEIGHBOUR) = m_children.at(L0_DOWN_LEFT_INDEX).get();
							child->m_neighbours.at(RIGHT_NEIGHBOUR) = nullptr;

							if constexpr (3 == Dim) // octtree
							{
								child->m_neighbours.at(TOP_NEIGHBOUR) = m_children.at(L1_DOWN_RIGHT_INDEX).get();
								child->m_neighbours.at(BOTTOM_NEIGHBOUR) = nullptr;
							}
							break;


						case L0_DOWN_LEFT_INDEX:

							child->m_neighbours.at(UP_NEIGHBOUR) = m_children.at(L0_UP_LEFT_INDEX).get();
							child->m_neighbours.at(DOWN_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(LEFT_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(RIGHT_NEIGHBOUR) = m_children.at(L0_DOWN_RIGHT_INDEX).get();

							if constexpr (3 == Dim) // octtree
							{
								child->m_neighbours.at(TOP_NEIGHBOUR) = m_children.at(L1_DOWN_LEFT_INDEX).get();
								child->m_neighbours.at(BOTTOM_NEIGHBOUR) = nullptr;
							}
							break;


						// octtree from here
						case L1_UP_LEFT_INDEX:

							child->m_neighbours.at(UP_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(DOWN_NEIGHBOUR) = m_children.at(L1_DOWN_LEFT_INDEX).get();
							child->m_neighbours.at(LEFT_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(RIGHT_NEIGHBOUR) = m_children.at(L1_UP_RIGHT_INDEX).get();

							child->m_neighbours.at(TOP_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(BOTTOM_NEIGHBOUR) = m_children.at(L0_UP_LEFT_INDEX).get();
							break;

						case L1_UP_RIGHT_INDEX:

							child->m_neighbours.at(UP_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(DOWN_NEIGHBOUR) = m_children.at(L1_DOWN_RIGHT_INDEX).get();
							child->m_neighbours.at(LEFT_NEIGHBOUR) = m_children.at(L1_UP_LEFT_INDEX).get();
							child->m_neighbours.at(RIGHT_NEIGHBOUR) = nullptr;

							child->m_neighbours.at(TOP_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(BOTTOM_NEIGHBOUR) = m_children.at(L0_UP_RIGHT_INDEX).get();
							break;

						case L1_DOWN_RIGHT_INDEX:

							child->m_neighbours.at(UP_NEIGHBOUR) = m_children.at(L1_UP_RIGHT_INDEX).get();
							child->m_neighbours.at(DOWN_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(LEFT_NEIGHBOUR) = m_children.at(L1_DOWN_LEFT_INDEX).get();
							child->m_neighbours.at(RIGHT_NEIGHBOUR) = nullptr;

							child->m_neighbours.at(TOP_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(BOTTOM_NEIGHBOUR) = m_children.at(L0_DOWN_RIGHT_INDEX).get();
							break;

						case L1_DOWN_LEFT_INDEX:

							child->m_neighbours.at(UP_NEIGHBOUR) = m_children.at(L1_UP_LEFT_INDEX).get();
							child->m_neighbours.at(DOWN_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(LEFT_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(RIGHT_NEIGHBOUR) = m_children.at(L1_DOWN_RIGHT_INDEX).get();

							child->m_neighbours.at(TOP_NEIGHBOUR) = nullptr;
							child->m_neighbours.at(BOTTOM_NEIGHBOUR) = m_children.at(L0_DOWN_LEFT_INDEX).get();
							break;

						default:
							break;
					}
				
					if (!isRoot())
					{
						//complete children neighbours with neighbours's others childen					
						switch (i)
						{
							case L0_UP_LEFT_INDEX:

								if (m_neighbours.at(LEFT_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(LEFT_NEIGHBOUR)->m_children.at(L0_UP_RIGHT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(LEFT_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(RIGHT_NEIGHBOUR) = child;
									}
								}

								if(m_neighbours.at(UP_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(UP_NEIGHBOUR)->m_children.at(L0_DOWN_LEFT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(UP_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(DOWN_NEIGHBOUR) = child;
									}
								}

								if constexpr (3 == Dim) // octtree
								{
									if (m_neighbours.at(BOTTOM_NEIGHBOUR))
									{
										auto child_neighbour = m_neighbours.at(BOTTOM_NEIGHBOUR)->m_children.at(L1_UP_LEFT_INDEX).get();
										if (child_neighbour)
										{
											child->m_neighbours.at(BOTTOM_NEIGHBOUR) = child_neighbour;
											child_neighbour->m_neighbours.at(TOP_NEIGHBOUR) = child;
										}
									}
								}
								break;

							case L0_UP_RIGHT_INDEX:

								if (m_neighbours.at(RIGHT_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(RIGHT_NEIGHBOUR)->m_children.at(L0_UP_LEFT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(RIGHT_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(LEFT_NEIGHBOUR) = child;
									}
								}

								if (m_neighbours.at(UP_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(UP_NEIGHBOUR)->m_children.at(L0_DOWN_RIGHT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(UP_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(DOWN_NEIGHBOUR) = child;
									}
								}

								if constexpr (3 == Dim) // octtree
								{
									if (m_neighbours.at(BOTTOM_NEIGHBOUR))
									{
										auto child_neighbour = m_neighbours.at(BOTTOM_NEIGHBOUR)->m_children.at(L1_UP_RIGHT_INDEX).get();
										if (child_neighbour)
										{
											child->m_neighbours.at(BOTTOM_NEIGHBOUR) = child_neighbour;
											child_neighbour->m_neighbours.at(TOP_NEIGHBOUR) = child;
										}
									}
								}
								break;

							case L0_DOWN_RIGHT_INDEX:

								if (m_neighbours.at(RIGHT_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(RIGHT_NEIGHBOUR)->m_children.at(L0_DOWN_LEFT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(RIGHT_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(LEFT_NEIGHBOUR) = child;
									}
								}

								if (m_neighbours.at(DOWN_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(DOWN_NEIGHBOUR)->m_children.at(L0_UP_RIGHT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(DOWN_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(UP_NEIGHBOUR) = child;
									}
								}

								if constexpr (3 == Dim) // octtree
								{
									if (m_neighbours.at(BOTTOM_NEIGHBOUR))
									{
										auto child_neighbour = m_neighbours.at(BOTTOM_NEIGHBOUR)->m_children.at(L1_DOWN_RIGHT_INDEX).get();
										if (child_neighbour)
										{
											child->m_neighbours.at(BOTTOM_NEIGHBOUR) = child_neighbour;
											child_neighbour->m_neighbours.at(TOP_NEIGHBOUR) = child;
										}
									}
								}
								break;

							case L0_DOWN_LEFT_INDEX:

								if (m_neighbours.at(LEFT_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(LEFT_NEIGHBOUR)->m_children.at(L0_DOWN_RIGHT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(LEFT_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(RIGHT_NEIGHBOUR) = child;
									}
								}

								if (m_neighbours.at(DOWN_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(DOWN_NEIGHBOUR)->m_children.at(L0_UP_LEFT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(DOWN_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(UP_NEIGHBOUR) = child;
									}
								}

								if constexpr (3 == Dim) // octtree
								{
									if (m_neighbours.at(BOTTOM_NEIGHBOUR))
									{
										auto child_neighbour = m_neighbours.at(BOTTOM_NEIGHBOUR)->m_children.at(L1_DOWN_LEFT_INDEX).get();
										if (child_neighbour)
										{
											child->m_neighbours.at(BOTTOM_NEIGHBOUR) = child_neighbour;
											child_neighbour->m_neighbours.at(TOP_NEIGHBOUR) = child;
										}
									}
								}
								break;

							case L1_UP_LEFT_INDEX:

								if (m_neighbours.at(LEFT_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(LEFT_NEIGHBOUR)->m_children.at(L1_UP_RIGHT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(LEFT_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(RIGHT_NEIGHBOUR) = child;
									}
								}

								if (m_neighbours.at(UP_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(UP_NEIGHBOUR)->m_children.at(L1_DOWN_LEFT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(UP_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(DOWN_NEIGHBOUR) = child;
									}
								}

								if constexpr (3 == Dim) // octtree
								{
									if (m_neighbours.at(TOP_NEIGHBOUR))
									{
										auto child_neighbour = m_neighbours.at(TOP_NEIGHBOUR)->m_children.at(L0_UP_LEFT_INDEX).get();
										if (child_neighbour)
										{
											child->m_neighbours.at(TOP_NEIGHBOUR) = child_neighbour;
											child_neighbour->m_neighbours.at(BOTTOM_NEIGHBOUR) = child;
										}
									}
								}
								break;

							case L1_UP_RIGHT_INDEX:

								if (m_neighbours.at(RIGHT_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(RIGHT_NEIGHBOUR)->m_children.at(L1_UP_LEFT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(RIGHT_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(LEFT_NEIGHBOUR) = child;
									}
								}

								if (m_neighbours.at(UP_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(UP_NEIGHBOUR)->m_children.at(L1_DOWN_RIGHT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(UP_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(DOWN_NEIGHBOUR) = child;
									}
								}

								if constexpr (3 == Dim) // octtree
								{
									if (m_neighbours.at(TOP_NEIGHBOUR))
									{
										auto child_neighbour = m_neighbours.at(TOP_NEIGHBOUR)->m_children.at(L0_UP_RIGHT_INDEX).get();
										if (child_neighbour)
										{
											child->m_neighbours.at(TOP_NEIGHBOUR) = child_neighbour;
											child_neighbour->m_neighbours.at(BOTTOM_NEIGHBOUR) = child;
										}
									}
								}
								break;

							case L1_DOWN_RIGHT_INDEX:

								if (m_neighbours.at(RIGHT_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(RIGHT_NEIGHBOUR)->m_children.at(L1_DOWN_LEFT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(RIGHT_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(LEFT_NEIGHBOUR) = child;
									}
								}

								if (m_neighbours.at(DOWN_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(DOWN_NEIGHBOUR)->m_children.at(L1_UP_RIGHT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(DOWN_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(UP_NEIGHBOUR) = child;
									}
								}

								if constexpr (3 == Dim) // octtree
								{
									if (m_neighbours.at(TOP_NEIGHBOUR))
									{
										auto child_neighbour = m_neighbours.at(TOP_NEIGHBOUR)->m_children.at(L0_DOWN_RIGHT_INDEX).get();
										if (child_neighbour)
										{
											child->m_neighbours.at(TOP_NEIGHBOUR) = child_neighbour;
											child_neighbour->m_neighbours.at(BOTTOM_NEIGHBOUR) = child;
										}
									}
								}
								break;

							case L1_DOWN_LEFT_INDEX:

								if (m_neighbours.at(LEFT_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(LEFT_NEIGHBOUR)->m_children.at(L1_DOWN_RIGHT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(LEFT_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(RIGHT_NEIGHBOUR) = child;
									}
								}

								if (m_neighbours.at(DOWN_NEIGHBOUR))
								{
									auto child_neighbour = m_neighbours.at(DOWN_NEIGHBOUR)->m_children.at(L1_UP_LEFT_INDEX).get();
									if (child_neighbour)
									{
										child->m_neighbours.at(DOWN_NEIGHBOUR) = child_neighbour;
										child_neighbour->m_neighbours.at(UP_NEIGHBOUR) = child;
									}
								}

								if constexpr (3 == Dim) // octtree
								{
									if (m_neighbours.at(TOP_NEIGHBOUR))
									{
										auto child_neighbour = m_neighbours.at(TOP_NEIGHBOUR)->m_children.at(L0_DOWN_LEFT_INDEX).get();
										if (child_neighbour)
										{
											child->m_neighbours.at(TOP_NEIGHBOUR) = child_neighbour;
											child_neighbour->m_neighbours.at(BOTTOM_NEIGHBOUR) = child;
										}
									}
								}
								break;
						}
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

			std::vector<XTreeNode*> getNeighbours() const			
			{
				std::vector<XTreeNode*> neighbours;

				for (auto e : m_neighbours)
				{
					neighbours.push_back(e);
				}
				return neighbours;
			}

		private:

			static constexpr size_t two_pow(size_t p_exp)
			{
				size_t result = 1;
				for (size_t i = 0; i < p_exp; ++i) {
					result *= 2;
				}
				return result;
			}
			static constexpr int ChildCount{ two_pow(Dim) };

			XTreeNode* create_child(size_t p_index)
			{
				if (p_index < ChildCount)
				{
					if (m_children.at(p_index))
					{
						_EXCEPTION("children already exists")
					}
					m_children[p_index] = std::make_unique<XTreeNode>();
					m_children.at(p_index)->m_depth = m_depth + 1;
					m_children.at(p_index)->m_parent = this;
					m_children.at(p_index)->m_id = p_index;

					return m_children.at(p_index).get();
				}
				return nullptr;
			}

			void init_neighbours()
			{
				for (size_t i = 0; i < m_neighbours.size(); i++)
				{
					m_neighbours.at(i) = nullptr;
				}
			}

			NodeData												m_data;
			std::array<std::unique_ptr<XTreeNode>, ChildCount>		m_children;
			size_t													m_depth { 0 };

			std::array<XTreeNode*, Dim * 2>							m_neighbours; // quadtree : 2 * 2 -> 4 neighbours; octtree 3 * 2 -> 6 neighbours

			XTreeNode*												m_parent { nullptr };
			unsigned int											m_id;
		};

		template <typename NodeData>
		using QuadTreeNode = XTreeNode<2, NodeData>;

		template <typename NodeData>
		using OctreeNode = XTreeNode<3, NodeData>;

	} // core
} // mage
