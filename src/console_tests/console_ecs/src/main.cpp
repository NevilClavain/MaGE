
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
#include "aspects.h"
#include "system.h"
#include "sysengine.h"

using namespace mage;


class Foo
{
public:
	Foo() = default;
	~Foo()
	{
		std::cout << "Foo ctor call\n";
	}
};

int main( int argc, char* argv[] )
{    
	std::cout << "ECS tests\n";

	///// basic tree construction
	///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////

	{

		std::cout << "**** Entitygraph test\n\n";

		core::Entitygraph eg;

		std::cout << "eg has root : " << eg.hasRoot() << "\n";
		
		//auto& root_node{ eg.makeRoot("root") };		
		eg.makeRoot("root");
		auto& root_node{ eg.node("root") };


		const auto root_entity{ root_node.data() };


		root_entity->makeAspect(core::renderingAspect::id);

		{
			// write component in an entity/aspect

			auto& aspect{ root_entity->aspectAccess(core::renderingAspect::id) };
			aspect.addComponent<int>("width", 640);
		}

		{
			// read component component in an entity/aspect

			auto& aspect{ root_entity->aspectAccess(core::renderingAspect::id) };
			const auto width{ aspect.getComponent<int>("width")->getPurpose() };
			std::cout << "width = " << width << "\n";
		}

		std::cout << "eg has root now : " << eg.hasRoot() << "\n";

		eg.add(root_node, "ent1");
		eg.add(root_node, "ent2");

		auto& ent1{ eg.node("ent1") };

		ent1.data()->makeAspect(core::teapotAspect::id);
		{
			// write component in an entity/aspect
			auto& teapot_aspect{ ent1.data()->aspectAccess(core::teapotAspect::id) };
			teapot_aspect.addComponent<Foo>("foo");
		}

		eg.add( eg.add(ent1, "ent11"), "ent111");		
		eg.add(ent1, "ent12");


		auto& ent2{ eg.node("ent2") };
		eg.add(ent2, "ent21");


		std::cout << "////////////////////////////////////\n\n";

		std::cout << "root to leaf browsing\n";

		// root to leaf browsing
		for (auto it = eg.preBegin(); it != eg.preEnd(); ++it)
		{
			const auto currId{ it->data()->getId() };

			for (int i = 0; i < it->data()->getDepth(); i++) std::cout << " ";
			std::cout << currId << "\n";
		}
		std::cout << "\n";

		std::cout << "leaf to root browsing\n";

		// leaf to root browsing
		for (auto it = eg.postBegin(); it != eg.postEnd(); ++it)
		{
			const auto currId{ it->data()->getId() };

			for (int i = 0; i < it->data()->getDepth(); i++) std::cout << " ";
			std::cout << currId << "\n";
		}
		std::cout << "\n";
		
		// remove a node
		eg.remove(eg.node("ent21"));
		eg.remove(eg.node("ent2"));

		std::cout << "root to leaf browsing\n";
		// root to leaf browsing
		for (auto it = eg.preBegin(); it != eg.preEnd(); ++it)
		{
			const auto currId{ it->data()->getId() };

			for (int i = 0; i < it->data()->getDepth(); i++) std::cout << " ";
			std::cout << currId << "\n";
		}

		std::cout << "////////////////////////////////////\n";
		std::cout << "parent insertion test\n\n";

		auto& ent111{ eg.node("ent111") };

		eg.insertParent(ent111, "ent11_inserted");

		std::cout << "root to leaf browsing\n";

		// root to leaf browsing
		for (auto it = eg.preBegin(); it != eg.preEnd(); ++it)
		{
			const auto currId{ it->data()->getId() };

			for (int i = 0; i < it->data()->getDepth(); i++) std::cout << " ";
			std::cout << currId << "\n";
		}
		std::cout << "\n";



	}
    return 0;
}
