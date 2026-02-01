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
include_directories(${CMAKE_SOURCE_DIR}/CORE_maths/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_time/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_threads/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_filesystem/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_services/src)
include_directories(${CMAKE_SOURCE_DIR}/CORE_xtree/src)

include_directories(${CMAKE_SOURCE_DIR}/RENDERING_control/src)
include_directories(${CMAKE_SOURCE_DIR}/TRANSFORM_control/src)

include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_d3d11/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_time/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_resource/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_renderingqueue/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_world/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_dataprint/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_animations/src)
include_directories(${CMAKE_SOURCE_DIR}/SYSTEM_scenestreamer/src)

include_directories(${CMAKE_SOURCE_DIR}/helpers)

include_directories(${CMAKE_SOURCE_DIR}/appbases/src/base)
include_directories(${CMAKE_SOURCE_DIR}/appbases/src/openenv)
include_directories(${CMAKE_SOURCE_DIR}/appbases/src/streamed_openenv)

include_directories(${st_tree_include_dir})
include_directories(${jsonstruct_include_dir})


file(GLOB_RECURSE BASE "${CMAKE_SOURCE_DIR}/appbases/src/base/*.*")
file(GLOB_RECURSE OPENENV "${CMAKE_SOURCE_DIR}/appbases/src/openenv/*.*")
file(GLOB_RECURSE STREAMED_OPENENV "${CMAKE_SOURCE_DIR}/appbases/src/streamed_openenv/*.*")

source_group("base" FILES ${BASE})
source_group("openenv" FILES ${OPENENV})
source_group("streamed_openenv" FILES ${STREAMED_OPENENV})

add_definitions( -D_FROMCMAKE )

add_library(appbases ${BASE} ${OPENENV} ${STREAMED_OPENENV})


