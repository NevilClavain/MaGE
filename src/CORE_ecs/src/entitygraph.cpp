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

	m_entites[p_entity_id] = std::make_unique<Entity>(p_entity_id, p_parent.data());

	NodeIterator ite_new_node{ p_parent.insert(&*(m_entites[p_entity_id].get())) };

	if (m_nodes.count(p_entity_id))
	{
		_EXCEPTION("entity already exists : " + p_entity_id);
	}

	m_nodes[p_entity_id] = &*ite_new_node;

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
