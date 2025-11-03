
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
#include <string>
#include <map>
#include "xtree.h"

using namespace mage::core;


void print_neighbours(QuadTreeNode<std::string>* node)
{
	std::cout << "---------------------\n";
	std::cout << "neigbours of " << node->getData() << "\n";

	const std::vector<QuadTreeNode<std::string>*> neighbours{ node->getNeighbours() };

	const std::map<int, std::string> neighb_index
	{
		{ 0, "UP_NEIGHBOUR" },
		{ 1, "DOWN_NEIGHBOUR" },
		{ 2, "LEFT_NEIGHBOUR" },
		{ 3, "RIGHT_NEIGHBOUR" },
		{ 4, "TOP_NEIGHBOUR" },
		{ 5, "BOTTOM_NEIGHBOUR" }
	};

	for (size_t i = 0; i < neighbours.size(); i++)
	{
		std::string data;

		if (nullptr == neighbours.at(i))
		{
			data = "(null)";
		}
		else
		{
			data = neighbours.at(i)->getData();
		}

		std::cout << neighb_index.at(i) << " : " << data << "\n";
	}

	std::cout << "\n";
	std::cout << "---------------------\n";
}

int main( int argc, char* argv[] )
{    
	std::cout << "XTree test !\n";

	QuadTreeNode<std::string> root("root");

	root.split();

	root.getChild(0)->setData("<0>");
	root.getChild(1)->setData("<1>");
	root.getChild(2)->setData("<2>");
	root.getChild(3)->setData("<3>");


	root.getChild(0)->split();
	root.getChild(0)->getChild(0)->setData("<0_0>");
	root.getChild(0)->getChild(1)->setData("<0_1>");
	root.getChild(0)->getChild(2)->setData("<0_2>");
	root.getChild(0)->getChild(3)->setData("<0_3>");

	root.getChild(1)->split();
	root.getChild(1)->getChild(0)->setData("<1_0>");
	root.getChild(1)->getChild(1)->setData("<1_1>");
	root.getChild(1)->getChild(2)->setData("<1_2>");
	root.getChild(1)->getChild(3)->setData("<1_3>");

	root.getChild(2)->split();
	root.getChild(2)->getChild(0)->setData("<2_0>");
	root.getChild(2)->getChild(1)->setData("<2_1>");
	root.getChild(2)->getChild(2)->setData("<2_2>");
	root.getChild(2)->getChild(3)->setData("<2_3>");


	root.getChild(3)->split();
	root.getChild(3)->getChild(0)->setData("<3_0>");
	root.getChild(3)->getChild(1)->setData("<3_1>");
	root.getChild(3)->getChild(2)->setData("<3_2>");
	root.getChild(3)->getChild(3)->setData("<3_3>");


	//root.merge();
	//root.getChild(3)->merge();

	////////////////////////////////////////////

	root.traverse([](const std::string& p_data, size_t p_depth) 
	{		
		for (size_t i = 0; i < p_depth; i++) std::cout << " ";
		std::cout << "depth " << p_depth << " value = " << p_data << "\n";
	}); 

	//print_neighbours(&root);
	//print_neighbours(root.getChild(0));
	//print_neighbours(root.getChild(2));

	print_neighbours(root.getChild(3)->getChild(1));



    return 0;
}
