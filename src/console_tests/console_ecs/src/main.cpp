
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







		/*
		std::cout << "////////////////////////////////////\n\n";
		std::cout << "move_subtree test\n";



		eg.move_subtree(eg.node("ent21"), eg.node("ent1"));

		std::cout << "root to leaf browsing\n";

		// root to leaf browsing
		for (auto it = eg.preBegin(); it != eg.preEnd(); ++it)
		{
			const auto currId{ it->data()->getId() };

			for (int i = 0; i < it->data()->getDepth(); i++) std::cout << " ";
			std::cout << currId << "\n";
		}
		std::cout << "\n";
		*/



		//std::cout << "leaf to root browsing\n";

		//// leaf to root browsing
		//for (auto it = eg.postbegin(); it != eg.postend(); ++it)
		//{
		//	const auto currid{ it->data()->getid() };

		//	for (int i = 0; i < it->data()->getdepth(); i++) std::cout << " ";
		//	std::cout << currid << "\n";
		//}
		//std::cout << "\n";
		
		// remove a node
		//eg.remove(eg.node("ent111"));

		//std::cout << "root to leaf browsing\n";
		//// root to leaf browsing
		//for (auto it = eg.preBegin(); it != eg.preEnd(); ++it)
		//{
		//	const auto currid{ it->data()->getId() };

		//	for (int i = 0; i < it->data()->getDepth(); i++) std::cout << " ";
		//	std::cout << currid << "\n";
		//}

		//std::cout << "remove parent test\n\n";
		//{
		//	auto& ent111{ eg.node("ent111") };

		//	eg.removeParent(ent111);

		//	// root to leaf browsing
		//	for (auto it = eg.preBegin(); it != eg.preEnd(); ++it)
		//	{
		//		const auto currId{ it->data()->getId() };

		//		for (int i = 0; i < it->data()->getDepth(); i++) std::cout << " ";
		//		std::cout << currId << "\n";
		//	}
		//	std::cout << "\n";
		//}


		std::cout << "////////////////////////////////////\n\n";
		std::cout << "all teapot aspects : \n";

		auto& ent21{ eg.node("ent21") };
		ent21.data()->makeAspect(core::teapotAspect::id);

		auto& ent12{ eg.node("ent12") };
		ent12.data()->makeAspect(core::teapotAspect::id);


		auto teapots_entities{ eg.getEntitiesListForAspect(core::teapotAspect::id) };

		for (auto& e : teapots_entities)
		{
			std::cout << "entity : " << e->getId() << "\n";
		}
		std::cout << "\n";

		eg.remove(eg.node("ent12"));
		eg.remove(eg.node("ent21"));

		std::cout << "all teapot aspects after removing: \n";

		// remove teapot entities

		teapots_entities = eg.getEntitiesListForAspect(core::teapotAspect::id);

		for (auto& e : teapots_entities)
		{
			std::cout << "entity : " << e->getId() << "\n";
		}
		std::cout << "\n";

		//////////////
		// check we can have void returned list
		auto animated_entities{ eg.getEntitiesListForAspect(core::animationsAspect::id) }; // animated_entities size is : 0

	}
    return 0;
}
