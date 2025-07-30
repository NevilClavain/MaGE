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

#include <iostream>

#include "entitygraph.h"
#include "entity.h"
#include "exceptions.h"

using namespace mage::core;

Entitygraph::Node& Entitygraph::makeRoot(const std::string& p_entity_id)
{
	if (hasRoot())
	{
		_EXCEPTION("Entitygraph root already set")
	}

	m_entites[p_entity_id] = std::make_unique<Entity>(p_entity_id);
	m_tree.insert(m_entites.at(p_entity_id).get());

	m_rootNodeName = p_entity_id; // later we can access to root node through (see bellow)

	for (const auto& call : m_callbacks)
	{
		call(EntitygraphEvents::ENTITYGRAPHNODE_ADDED, *m_entites.at(p_entity_id).get());
	}

	m_nodes[p_entity_id] = &m_tree.root();

	return m_tree.root();
}

bool Entitygraph::hasRoot() const
{
	return (m_tree.depth() > 0);
}

Entitygraph::Node& Entitygraph::add(Node& p_parent, const std::string& p_entity_id)
{
	const auto parent_id{ p_parent.data()->getId() };

	if (0 == m_entites.count(parent_id))
	{
		_EXCEPTION("parent not registered : " + parent_id)
	}

	if (m_nodes.count(p_entity_id))
	{
		_EXCEPTION("entity already exists : " + p_entity_id);
	}

	m_entites[p_entity_id] = std::make_unique<Entity>(p_entity_id, p_parent.data());

	NodeIterator ite_new_node{ p_parent.insert(&*(m_entites[p_entity_id].get())) };

	m_nodes[p_entity_id] = &*ite_new_node;

	m_entites.at(p_entity_id)->m_depth = p_parent.data()->m_depth + 1;

	for (const auto& call : m_callbacks)
	{
		call(EntitygraphEvents::ENTITYGRAPHNODE_ADDED, *m_entites.at(p_entity_id).get());
	}
	return *ite_new_node;
}

void Entitygraph::remove(Node& p_node)
{
	const auto& entity{ *p_node.data() };

	if (!p_node.empty())
	{
		_EXCEPTION("Cant remove entity " + entity.getId() + " : it has children");
	}

	for (const auto& call : m_callbacks)
	{
		call(EntitygraphEvents::ENTITYGRAPHNODE_REMOVED, entity);
	}

	p_node.erase();
	const auto id{ entity.getId() };

	m_nodes.erase(id);
	m_entites.erase(id);
}

void Entitygraph::move_subtree(Node& p_parent_dest, Node& p_src)
{
	st_tree::tree<core::Entity*> temp_tree;

	const auto& entity{ *p_src.data() };
	const auto entity_id{ entity.getId() };

	temp_tree.insert(m_entites.at(entity_id).get());

	const std::function<void(Node&, Node&)> clone_subtree
	{
		[&] (Node& p_src, Node& p_parent_dest)
		{
			for (auto it = m_tree.df_pre_begin(); it != m_tree.df_pre_end(); ++it)
			{
				if (!it->is_root() && it->parent() == p_src) 
				{
					const auto& entity{ it->data()};
					const auto entity_id{ entity->getId() };

					auto new_node{ p_parent_dest.insert(m_entites.at(entity_id).get()) };

					clone_subtree(*it, *new_node);
				}
			}
		}
	};

	clone_subtree(p_src, temp_tree.root());

	p_src.erase();
	m_nodes.erase(entity_id);

	auto new_node{ p_parent_dest.insert(m_entites.at(entity_id).get()) };
	m_nodes[entity_id] = &*new_node;
	m_entites.at(entity_id)->m_depth = p_parent_dest.data()->m_depth + 1;

	m_entites.at(entity_id)->m_parent = p_parent_dest.data();
	
	const std::function<void(Node&, Node&)> restore_subtree
	{
		[&](Node& p_src, Node& p_parent_dest)
		{
			for (auto it = temp_tree.df_pre_begin(); it != temp_tree.df_pre_end(); ++it)
			{
				if (!it->is_root() && it->parent() == p_src)
				{
					const auto& entity{ it->data()};
					const auto entity_id{ entity->getId() };

					auto new_node { p_parent_dest.insert(m_entites.at(entity_id).get()) };

					m_nodes[entity_id] = &*new_node ;
					m_entites.at(entity_id)->m_depth = p_parent_dest.data()->m_depth + 1;

					restore_subtree(*it, *new_node);
				}
			}
		}
	};

	restore_subtree(temp_tree.root(), *new_node);
}



Entitygraph::Node& Entitygraph::node(const std::string& p_entity_id)
{
	const auto id{ p_entity_id };

	if (0 == m_nodes.count(id))
	{
		_EXCEPTION("node not registered " + id)
	}
	return (*m_nodes.at(id));		
}

Entitygraph::PreIterator Entitygraph::preBegin()
{
	return m_tree.df_pre_begin();
}

Entitygraph::PreIterator Entitygraph::preEnd()
{
	return m_tree.df_pre_end();
}

Entitygraph::PostIterator Entitygraph::postBegin()
{
	return m_tree.df_post_begin();
}

Entitygraph::PostIterator Entitygraph::postEnd()
{
	return m_tree.df_post_end();
}
