
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
#include <json_struct/json_struct.h>
#include <vector>

const char data_0[] = R"json(
{
    "key" : 4, "value": 1.0
}
)json";


const char data_1[] = R"json(
{
    "id" : "main",
    "vec":
    [
        0.1,
        0.2,
        0.3,
        0.45555
    ],
    "keys" : [
        { "key" : 4, "value": 1.0 },
        { "key" : 5, "value": 2.0 },
        { "key" : 6, "value": 3.0 }
    ]
}
)json";


const char json_graph[] = R"json(
{
    "entity_name" : "root",


    "children" : 
    [
        { 
            "entity_name" : "pass1",
            "children" : 
            [
                { 
                    "entity_name" : "subpass1",
                    "children" : []
                }
            ]
        },
        { "entity_name" : "pass2",
            "children" : []
        }
    ]
}
)json";



struct Data_0
{
    std::string key;
    double value = 0.0;

    JS_OBJ(key, value);
};

struct Data_1
{
    std::string         id;
    std::vector<Data_0> keys;

    double              vec[4];

    JS_OBJ(id, keys, vec);
};

struct TestEntity
{
    std::string             entity_name;
    std::vector<TestEntity> children;

    JS_OBJ(entity_name, children);
};



int json_err(const JS::ParseContext& p_pc)
{
    std::string errorStr = p_pc.makeErrorString();
    std::cout << "Error parsing struct :" << errorStr.c_str() << "\n";
    return -1;
}

int main( int argc, char* argv[] )
{    
	std::cout << "json_struct test !\n";

    //////////////////////////////////////////////////
    {
        Data_0 obj;

        JS::ParseContext parseContext(data_0);
        if (parseContext.parseTo(obj) != JS::Error::NoError)
        {
            return json_err(parseContext);
        }

        std::cout << obj.key << " " << obj.value << "\n";

        Data_0 obj2 = obj;

        obj2.key = "clone";
        obj2.value = 666;

        std::cout << "json serialisation : \n";
        std::cout << JS::serializeStruct(obj2) << "\n";
    }
    std::cout << "/////////////////////////////////////////////\n";

    //////////////////////////////////////////////////
    {
        Data_1 obj;

        JS::ParseContext parseContext(data_1);
        if (parseContext.parseTo(obj) != JS::Error::NoError)
        {
            return json_err(parseContext);
        }

        std::cout << obj.id << "\n";

        for (const auto& e : obj.keys)
        {
            std::cout << e.key << " " << e.value << "\n";
        }

        for (int i = 0; i < 4; i++)
        {
            std::cout << obj.vec[i] << " ";
        }
        std::cout << "\n";
        
    }
    std::cout << "/////////////////////////////////////////////\n";

    //////////////////////////////////////////////////

    {
        TestEntity graph;

        JS::ParseContext parseContext(json_graph);
        if (parseContext.parseTo(graph) != JS::Error::NoError)
        {
            return json_err(parseContext);
        }

        std::cout << graph.entity_name << "\n";
        std::cout << "  " << graph.children.at(0).entity_name << "\n";
        std::cout << "    " << graph.children.at(0).children.at(0).entity_name << "\n";
    }



    return 0;
}
 