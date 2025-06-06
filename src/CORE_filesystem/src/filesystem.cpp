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

#include <windows.h>
#include "filesystem.h"
#include "exceptions.h"

using namespace mage::core;

bool fileSystem::exists(const std::string& p_path)
{
    const auto dwAttrib{ GetFileAttributes(p_path.c_str()) };
    return (dwAttrib != INVALID_FILE_ATTRIBUTES);
}

bool fileSystem::isDirectory(const std::string& p_path)
{
    const auto dwAttrib{ GetFileAttributes(p_path.c_str()) };
    return (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

void fileSystem::createDirectory(const std::string& p_path)
{
    if(!::CreateDirectory(p_path.c_str(), nullptr))
    {
        _EXCEPTION("Failed to create directory : " + p_path);
    }
}

long fileSystem::fileSize(FILE* p_fp)
{  
    const auto current_pos{ ftell(p_fp) };
    fseek(p_fp, 0, SEEK_END);
    const auto size{ ftell(p_fp) };
    fseek(p_fp, current_pos, SEEK_SET);
    return size;
}

std::pair<std::string, std::string> fileSystem::splitFilename(const std::string& p_filename)
{
    const auto extension { std::filesystem::path(p_filename).extension().string() };
    const auto name{ std::filesystem::path(p_filename).stem().string() };

    return std::make_pair(name, extension);
}

std::pair<std::string, std::string> fileSystem::splitPath(const std::string& p_path)
{
    const auto fn{ std::filesystem::path(p_path).filename().string() };
    const auto folder{ std::filesystem::path(p_path).parent_path().string() };

    return std::make_pair(folder, fn);
}

