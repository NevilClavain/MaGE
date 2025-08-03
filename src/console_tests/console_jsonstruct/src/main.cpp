
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

const char data_0[] = R"json(
{
    "key" : 4, "value": 1.0
}
)json";

struct Data_0
{
    std::string key;
    double value = 0.0;

    JS_OBJ(key, value);
};

int main( int argc, char* argv[] )
{    
	std::cout << "json_struct test !\n";

    //////////////////////////////////////////////////

    {

        Data_0 obj;

        JS::ParseContext parseContext(data_0);
        if (parseContext.parseTo(obj) != JS::Error::NoError)
        {
            std::string errorStr = parseContext.makeErrorString();
            std::cout << "Error parsing struct :" << errorStr.c_str() << "\n";
            return -1;
        }

        std::cout << obj.key << " " << obj.value << "\n";
    }

    return 0;
}
 