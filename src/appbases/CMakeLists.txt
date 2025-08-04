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
project(appbases)

include_directories(${CMAKE_SOURCE_DIR}/commons)
include_directories(${CMAKE_SOURCE_DIR}/CORE_ecs/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_allocator/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_logger/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_filesystem/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_buffer/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_json/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_maths/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_time/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_threads/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_filesystem/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_services/src)

include_directories(${CMAKE_SOURCE_DIR}/RENDERING_control/src)
include_directories(${CMAKE_SOURCE_DIR}/TRANSFORM_control/src)

include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_d3d11/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_time/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_resource/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_renderingqueue/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_world/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_dataprint/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_animations/src)

include_directories(${CMAKE_SOURCE_DIR}/helpers)

include_directories(${CMAKE_SOURCE_DIR}/appbases/src/samples_base)
include_directories(${CMAKE_SOURCE_DIR}/appbases/src/samples_openenv)

include_directories(${st_tree_include_dir})
include_directories(${jsonjsmn_include_dir})


file(GLOB_RECURSE SAMPLES_BASE "${CMAKE_SOURCE_DIR}/appbases/src/samples_base/*.*")
file(GLOB_RECURSE SAMPLES_OPENENV "${CMAKE_SOURCE_DIR}/appbases/src/samples_openenv/*.*")

source_group("samples_base" FILES ${SAMPLES_BASE})
source_group("samples_openenv" FILES ${SAMPLES_OPENENV})

add_definitions( -D_FROMCMAKE )

add_library(appbases ${SAMPLES_BASE} ${SAMPLES_OPENENV})


