
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
#include "xtree.h"

using namespace mage::core;


int main( int argc, char* argv[] )
{    
	std::cout << "XTree test !\n";

	QuadTreeNode<std::string> root("root");

	root.split();

	root.getChild(0)->setData("index 0");
	root.getChild(1)->setData("index 1");
	root.getChild(2)->setData("index 2");
	root.getChild(3)->setData("index 3");


	root.getChild(0)->split();
	root.getChild(0)->getChild(0)->setData("index 0");
	root.getChild(0)->getChild(1)->setData("index 1");
	root.getChild(0)->getChild(2)->setData("index 2");
	root.getChild(0)->getChild(3)->setData("index 3");

	root.getChild(3)->split();
	root.getChild(3)->getChild(0)->setData("index 0");
	root.getChild(3)->getChild(1)->setData("index 1");
	root.getChild(3)->getChild(2)->setData("index 2");
	root.getChild(3)->getChild(3)->setData("index 3");


	//root.merge();
	//root.getChild(3)->merge();

	////////////////////////////////////////////

	root.traverse([](const std::string& p_data, size_t p_depth) 
	{
		for (size_t i = 0; i < p_depth; i++) std::cout << " ";
		std::cout << "depth " << p_depth << " value = " << p_data << "\n";
	}); 

    return 0;
}
