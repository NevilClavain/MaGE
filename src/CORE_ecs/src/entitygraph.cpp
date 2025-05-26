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

Entitygraph::Node& Entitygraph::insertParent(Node& p_node, const std::string& p_parent_entity_id)
{
	const auto& entity{ *p_node.data() };
	const auto entity_id{ entity.getId() };

	if (!m_entites.count(entity_id))
	{
		_EXCEPTION("node not registered : " + entity_id)
	}
	
	// do not support operation if node to move is the root
	if (!entity.getParent())
	{
		_EXCEPTION("refused because " + entity_id + " is root")
	}

	// leaf only
	if (!p_node.empty())
	{
		_EXCEPTION("Cant move entity " + entity.getId() + " : it has children");
	}

	// retrieve node parent
	Entity* original_parent_entity{ entity.getParent()};
	const auto original_parent_entity_id{ original_parent_entity->getId() };

	p_node.erase(); // erase the node to move
	m_nodes.erase(entity_id);

	Node* original_parent_node{ nullptr };
	for (const auto& e : m_nodes)
	{
		Entity* curr_entity = e.second->data();
		if (e.second->data()->getId() == original_parent_entity_id)
		{
			original_parent_node = e.second;
		}
	}
	
	auto& inserted_parent_node{ add(*original_parent_node, p_parent_entity_id) };

	
	NodeIterator ite_new_node{ inserted_parent_node.insert(&*(m_entites.at(entity_id).get())) };
	m_nodes[entity_id] = &*ite_new_node;

	m_entites.at(entity_id)->m_depth = inserted_parent_node.data()->m_depth + 1;
	m_entites.at(entity_id)->m_parent = inserted_parent_node.data();
	
	return inserted_parent_node;
}

void Entitygraph::removeParent(Node& p_node)
{
	const auto& entity{ *p_node.data() };
	const auto entity_id{ entity.getId() };

	if (!m_entites.count(entity_id))
	{
		_EXCEPTION("node not registered : " + entity_id)
	}

	// do not support operation if node to move is the root
	if (!entity.getParent())
	{
		_EXCEPTION("refused because " + entity_id + " is root")
	}

	// retrieve node parent
	Entity* parent_entity{ entity.getParent() };
	const auto parent_entity_id{ parent_entity->getId() };

	// do not support operation also if parent of node to move is the root
	if (!parent_entity->getParent())
	{
		_EXCEPTION("refused because " + entity_id + " parent is root")
	}

	// leaf only
	if (!p_node.empty())
	{
		_EXCEPTION("Cant move entity " + entity_id + " : it has children");
	}

	// retrieve node upper parent

	p_node.erase(); // erase the node to move
	m_nodes.erase(entity_id);

	// and move this node under upper parent

	Entity* upper_parent_entity{ parent_entity->getParent() };
	const auto upper_parent_entity_id{ upper_parent_entity->getId() };

	Node* upper_parent_node{ nullptr };
	for (const auto& e : m_nodes)
	{
		Entity* curr_entity = e.second->data();
		if (e.second->data()->getId() == upper_parent_entity_id)
		{
			upper_parent_node = e.second;
		}
	}


	NodeIterator ite_new_node{ upper_parent_node->insert(&*(m_entites.at(entity_id).get())) };
	m_nodes[entity_id] = &*ite_new_node;

	m_entites.at(entity_id)->m_depth = upper_parent_node->data()->m_depth + 1;
	m_entites.at(entity_id)->m_parent = upper_parent_node->data();

	// then finally remove previous node parent

	Node* parent_node{ nullptr };
	for (const auto& e : m_nodes)
	{
		Entity* curr_entity = e.second->data();
		if (e.second->data()->getId() == parent_entity_id)
		{
			parent_node = e.second;
		}
	}

	remove(*parent_node);
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
