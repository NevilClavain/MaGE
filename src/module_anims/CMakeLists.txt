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
project(module_anims)

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

link_directories(${jsonjsmn_lib_dir})
link_directories(${fw1fontwrapper_lib_dir})
link_directories(${dxtk_lib_dir})
link_directories(${assimp_lib_dir})

file(
        GLOB_RECURSE
        source_files
		${CMAKE_SOURCE_DIR}/module_anims/src/*.h
        ${CMAKE_SOURCE_DIR}/module_anims/src/*.cpp
		
)


add_library(module_anims SHARED ${source_files})

target_link_libraries(module_anims 
                                        assimp-vc140-mt
										CORE_file 
										CORE_filesystem 
										CORE_buffer 
										CORE_logger 
										CORE_json 
										CORE_module 
										CORE_ecs 
										CORE_allocator 
										CORE_time 
										CORE_threads 
										CORE_services 
										CORE_maths 
										helpers
										appbases
										RENDERING_control 
										TRANSFORM_control 
										SYSTEM_d3d11 
										SYSTEM_time 
										SYSTEM_resource 
										SYSTEM_renderingqueue 
										SYSTEM_world 
										SYSTEM_dataprint 
										SYSTEM_animations
										jsmn 
										d3d11 
										d3dcompiler 
										DirectXTK)

install(TARGETS module_anims CONFIGURATIONS Debug RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/apps/Debug)
install(TARGETS module_anims CONFIGURATIONS Release RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/apps/Release)
install(TARGETS module_anims CONFIGURATIONS RelWithDebInfo RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/apps/RelWithDebInfo)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/config_deployment/module_anims_config DESTINATION ${CMAKE_SOURCE_DIR}/apps/$<CONFIGURATION>)






