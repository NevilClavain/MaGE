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

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "singleton.h"

namespace mage
{
	namespace core
	{
		// fwd decl
		namespace logger { struct Sink; };

		namespace services
		{
			class LoggerSharing : public property::Singleton<LoggerSharing>
			{
			public:
				LoggerSharing() = default;
				~LoggerSharing() = default;

				void createLogger(const std::string& p_id);
				mage::core::logger::Sink& getLogger(const std::string& p_id);

			private:				
				std::unordered_map<std::string, std::unique_ptr<mage::core::logger::Sink>> m_loggers;
			};
		}
	}
}
