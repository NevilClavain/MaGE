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
project(CORE_json)

include_directories(${CMAKE_SOURCE_DIR}/commons)
include_directories(${jsonjsmn_include_dir})

file(
        GLOB_RECURSE
        source_files
		${CMAKE_SOURCE_DIR}/CORE_json/src/*.h
        ${CMAKE_SOURCE_DIR}/CORE_json/src/*.cpp		
)

add_definitions( -D_FROMCMAKE )

add_library(CORE_json ${source_files})



