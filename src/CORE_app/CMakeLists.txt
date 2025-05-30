# -*-LIC_BEGIN-*-
#                                                                          
# MaGE rendering framework
# Emmanuel Chaumont Copyright (c) 2023
#                                                                          
# This file is part of MaGE.                                          
#                                                                          
#    MaGE is free software: you can redistribute it and/or modify     
#    it under the terms of the GNU General Public License as published by  
#    the Free Software Foundation, either version 3 of the License, or     
#    (at your option) any later version.                                   
#                                                                          
#    MaGE is distributed in the hope that it will be useful,          
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         
#    GNU General Public License for more details.                          
#                                                                          
#    You should have received a copy of the GNU General Public License     
#    along with MaGE.  If not, see <http://www.gnu.org/licenses/>.    
#
# -*-LIC_END-*-

cmake_minimum_required(VERSION 3.0)
project(CORE_app)

include_directories(${CMAKE_SOURCE_DIR}/commons)
include_directories(${CMAKE_SOURCE_DIR}/CORE_filesystem/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_buffer/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_logger/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_json/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_ecs/src)
#include_directories(${CMAKE_SOURCE_DIR}/CORE_allocator/src)

include_directories(${st_tree_include_dir})
include_directories(${jsonjsmn_include_dir})


file(
        GLOB_RECURSE
        source_files
		${CMAKE_SOURCE_DIR}/CORE_app/src/*.h
        ${CMAKE_SOURCE_DIR}/CORE_app/src/*.cpp		
)


add_definitions( -D_FROMCMAKE )

add_library(CORE_app ${source_files})



