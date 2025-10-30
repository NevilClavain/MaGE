
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

namespace mage
{
	namespace core
	{
		template <size_t ChildCount, typename NodeData>
		class XTreeNode
		{
		public:
			XTreeNode() : m_depth(0) {}
			explicit XTreeNode(const NodeData& data, size_t depth = 0) : m_data(data), m_depth(depth) {}

			void createChild(size_t index, const NodeData& childData)
			{
				if (index < ChildCount)
				{
					if (m_children[index])
					{
						_EXCEPTION("children already exists")
					}
					m_children[index] = std::make_unique<XTreeNode>(childData, m_depth + 1);
				}
			}

			void createChild(size_t index)
			{
				if (index < ChildCount)
				{
					if (m_children[index])
					{
						_EXCEPTION("children already exists")
					}
					m_children[index] = std::make_unique<XTreeNode>();
					m_children[index]->m_depth = m_depth + 1;
				}
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
					createChild(i);
				}
			}

			void merge()
			{
				for (size_t i = 0; i < ChildCount; ++i)
				{
					m_children[i].reset();
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
			NodeData m_data;
			std::array<std::unique_ptr<XTreeNode>, ChildCount> m_children;
			size_t m_depth{ 0 };
		};

		template <typename NodeData>
		using QuadTreeNode = XTreeNode<4, NodeData>;

		template <typename NodeData>
		using OctreeNode = XTreeNode<8, NodeData>;

	} // core
} // mage
