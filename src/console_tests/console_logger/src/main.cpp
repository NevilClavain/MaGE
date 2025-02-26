
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

#include "filesystem.h"
#include "json.h"
#include "exceptions.h"

#include "logsink.h"
#include "logconf.h"
#include "logging.h"

static mage::core::logger::Sink appLogger("MyTestApp", mage::core::logger::Configuration::getInstance());

int main( int argc, char* argv[] )
{    
	std::cout << "Logger test\n";

	try
	{
		mage::core::FileContent<char> fc("./console_logger_assets/log_conf.json");
		fc.load();

		const auto dataSize{ fc.getDataSize() };
		const std::string data(fc.getData(), dataSize);

		mage::core::Json jsonParser;

		// init logger

		jsonParser.registerSubscriber(mage::core::logger::Configuration::getInstance()->getCallback());
		const auto parseStatus{ jsonParser.parse(data) };
		std::cout << "Parser status = " << (parseStatus > -1 ? "OK" : "KO") << "\n";

		_MAGE_DEBUG(appLogger, "hello from logger test !");
		_MAGE_TRACE(appLogger, "trace log with a value : " + std::to_string(666));

		_MAGE_TRACE(appLogger, "trace log with another value : " << 42 );

		_MAGE_WARN(appLogger, "this is a warning : " << 3.1415 );
	}
	catch (const std::exception& e)
	{
		const auto what{ e.what() };
		std::cout << what << "\n";
	}

    return 0;
}
