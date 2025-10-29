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

cmake_minimum_required(VERSION 3.5)
project(console_xtree)

include_directories(${CMAKE_SOURCE_DIR}/commons)
include_directories(${CMAKE_SOURCE_DIR}/CORE_xtree/src)

file(
        GLOB_RECURSE
        source_files
        ${CMAKE_SOURCE_DIR}/console_tests/console_xtree/src/*.cpp
		
)

add_executable(console_xtree ${source_files})

target_link_libraries(console_xtree CORE_maths)

install(TARGETS console_xtree CONFIGURATIONS Debug RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/apps/Debug)
install(TARGETS console_xtree CONFIGURATIONS Release RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/apps/Release)
install(TARGETS console_xtree CONFIGURATIONS RelWithDebInfo RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/apps/RelWithDebInfo)

