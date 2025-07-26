
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

#include "samplesopenenv.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <list>

#include <chrono>

#include "aspects.h"

#include "d3d11system.h"
#include "timesystem.h"
#include "resourcesystem.h"
#include "worldsystem.h"
#include "dataprintsystem.h"
#include "renderingqueuesystem.h"


#include "sysengine.h"
#include "filesystem.h"
#include "logconf.h"

#include "logger_service.h"

#include "worldposition.h"
#include "animatorfunc.h"

#include "trianglemeshe.h"
#include "renderstate.h"

#include "animationbone.h"
#include "scenenode.h"


#include "syncvariable.h"
#include "animators_helpers.h"

#include "datacloud.h"

#include "shaders_service.h"
#include "textures_service.h"

#include "entitygraph_helpers.h"
#include "renderingpasses_helpers.h"
#include "shadows_helpers.h"

#include "maths_helpers.h"

using namespace mage;
using namespace mage::core;
using namespace mage::rendering;


void SamplesOpenEnv::init(const std::string p_appWindowsEntityName)
{
	SamplesBase::init(p_appWindowsEntityName);
	d3d11_system_events_openenv();
}


void SamplesOpenEnv::d3d11_system_events_openenv()
{
	const auto sysEngine{ SystemEngine::getInstance() };
	const auto d3d11System{ sysEngine->getSystem<mage::D3D11System>(d3d11SystemSlot) };

	const D3D11System::Callback d3d11_cb
	{
		[&, this](D3D11SystemEvent p_event, const std::string& p_id)
		{
			switch (p_event)
			{
				case D3D11SystemEvent::D3D11_WINDOW_READY:
				{
					auto& appwindowNode{ m_entitygraph.node(p_id) };
					const auto appwindow{ appwindowNode.data() };

					const auto& mainwindows_rendering_aspect{ appwindow->aspectAccess(mage::core::renderingAspect::id) };

					const float characteristics_v_width{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportWidth")->getPurpose()};
					const float characteristics_v_height{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportHeight")->getPurpose()};


					const auto dataCloud{ mage::rendering::Datacloud::getInstance() };

					const auto window_dims{ dataCloud->readDataValue<mage::core::maths::IntCoords2D>("std.window_resol") };

					const int w_width{ window_dims.x() };
					const int w_height{ window_dims.y() };


					//////////////////////////////////////////

					/////////// commons shaders params
					
					dataCloud->registerData<maths::Real4Vector>("texture_keycolor_ps.key_color");
					dataCloud->updateDataValue<maths::Real4Vector>("texture_keycolor_ps.key_color", maths::Real4Vector(0, 0, 0, 1));


					dataCloud->registerData<maths::Real4Vector>("std.ambientlight.color");
					dataCloud->updateDataValue<maths::Real4Vector>("std.ambientlight.color", maths::Real4Vector(0.33, 0.33, 0.33, 1));
					


					dataCloud->registerData<maths::Real4Vector>("skydome_emissive_color");
					dataCloud->updateDataValue<maths::Real4Vector>("skydome_emissive_color", maths::Real4Vector(1.0, 1.0, 1.0, 1));


					dataCloud->registerData<maths::Real4Vector>("std.black_color");
					dataCloud->updateDataValue<maths::Real4Vector>("std.black_color", maths::Real4Vector(0.0, 0.0, 0.0, 1));

					dataCloud->registerData<maths::Real4Vector>("white_color");
					dataCloud->updateDataValue<maths::Real4Vector>("white_color", maths::Real4Vector(1.0, 1.0, 1.0, 1));


					dataCloud->registerData<maths::Real4Vector>("std.light0.dir");
					dataCloud->updateDataValue<maths::Real4Vector>("std.light0.dir", maths::Real4Vector(0.6, -0.18, 0.1, 1));

					dataCloud->registerData<maths::Real4Vector>("std.fog.color");
					dataCloud->updateDataValue<maths::Real4Vector>("std.fog.color", maths::Real4Vector(0.8, 0.9, 1, 1));

					dataCloud->registerData<maths::Real4Vector>("std.fog.density");
					dataCloud->updateDataValue<maths::Real4Vector>("std.fog.density", maths::Real4Vector(0.009, 0, 0, 0));


					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_0");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_0", maths::Real4Vector(skydomeOuterRadius, skydomeInnerRadius, skydomeOuterRadius * skydomeOuterRadius, skydomeInnerRadius * skydomeInnerRadius));

					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_1");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_1", maths::Real4Vector(skydomeScaleDepth, 1.0 / skydomeScaleDepth, 1.0 / (skydomeOuterRadius - skydomeInnerRadius), (1.0 / (skydomeOuterRadius - skydomeInnerRadius)) / skydomeScaleDepth));

					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_2");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_2", maths::Real4Vector(1.0 / std::pow(skydomeWaveLength_x, 4.0), 1.0 / std::pow(skydomeWaveLength_y, 4.0), 1.0 / std::pow(skydomeWaveLength_z, 4.0), 0));

					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_3");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_3", maths::Real4Vector(skydomeKr, skydomeKm, 4.0 * skydomeKr * 3.1415927, 4.0 * skydomeKm * 3.1415927));

					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_4");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_4", maths::Real4Vector(skydomeSkyfromspace_ESun, skydomeSkyfromatmo_ESun, skydomeGroundfromspace_ESun, skydomeGroundfromatmo_ESun));

					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_5");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_5", maths::Real4Vector(0.0, 0.0, 0.0, 1));



					dataCloud->registerData<maths::Real4Vector>("shadow_bias");
					dataCloud->updateDataValue<maths::Real4Vector>("shadow_bias", maths::Real4Vector(0.005, 0, 0, 0));

					dataCloud->registerData<maths::Real4Vector>("shadowmap_resol");
					dataCloud->updateDataValue<maths::Real4Vector>("shadowmap_resol", maths::Real4Vector(2048, 0, 0, 0));


					create_openenv_scenegraph(p_id);

					create_openenv_rendergraph("screenRendering_Filter_DirectForward_Quad_Entity", w_width, w_height, characteristics_v_width, characteristics_v_height);


					// create passes default configs

					const auto renderingHelper{ mage::helpers::RenderingPasses::getInstance() };

					renderingHelper->registerPass("bufferRendering_Scene_TexturesChannel_Queue_Entity");
					renderingHelper->registerPass("bufferRendering_Scene_ZDepthChannel_Queue_Entity");
					renderingHelper->registerPass("bufferRendering_Scene_AmbientLightChannel_Queue_Entity");
					renderingHelper->registerPass("bufferRendering_Scene_LitChannel_Queue_Entity");
					renderingHelper->registerPass("bufferRendering_Scene_EmissiveChannel_Queue_Entity");

					
					// ground rendering
					{
						auto textures_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_TexturesChannel_Queue_Entity") };
						textures_channel_config.vshader = "scene_recursive_texture_vs";
						textures_channel_config.pshader = "scene_recursive_texture_ps";
						textures_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("grass08.jpg", Texture())) };
						textures_channel_config.rs_list.at(3).setOperation(RenderState::Operation::SETTEXTUREFILTERTYPE);
						textures_channel_config.rs_list.at(3).setArg("linear_uvwrap");

						auto zdepth_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ZDepthChannel_Queue_Entity") };
						zdepth_channel_config.vshader = "scene_zdepth_vs";
						zdepth_channel_config.pshader = "scene_zdepth_ps";

						auto ambientlight_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_AmbientLightChannel_Queue_Entity") };
						ambientlight_channel_config.vshader = "scene_flatcolor_vs";
						ambientlight_channel_config.pshader = "scene_flatcolor_ps";

						auto lit_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_LitChannel_Queue_Entity") };
						lit_channel_config.vshader = "scene_lit_vs";
						lit_channel_config.pshader = "scene_lit_ps";

						auto em_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_EmissiveChannel_Queue_Entity") };
						em_channel_config.vshader = "scene_flatcolor_vs";
						em_channel_config.pshader = "scene_flatcolor_ps";

						helpers::PassesDescriptors passesDescriptors =
						{
							// config
							{
								{ "bufferRendering_Scene_TexturesChannel_Queue_Entity", textures_channel_config },
								{ "bufferRendering_Scene_ZDepthChannel_Queue_Entity", zdepth_channel_config },
								{ "bufferRendering_Scene_AmbientLightChannel_Queue_Entity", ambientlight_channel_config },
								{ "bufferRendering_Scene_LitChannel_Queue_Entity", lit_channel_config },
								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity", em_channel_config }
							},

							{
							},

							{

								{ "bufferRendering_Scene_AmbientLightChannel_Queue_Entity",
									{
										{ std::make_pair("std.ambientlight.color", "color") }
									}
								},
								{ "bufferRendering_Scene_LitChannel_Queue_Entity",
									{
										{ std::make_pair("std.light0.dir", "light_dir") }
									}
								},
								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity",
									{
										{ std::make_pair("std.black_color", "color") }
									}
								}
							}
						};

						renderingHelper->registerToPasses(m_entitygraph, m_groundEntity, passesDescriptors);
					}

					// wall rendering
					{
						auto textures_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_TexturesChannel_Queue_Entity") };
						textures_channel_config.vshader = "scene_texture1stage_keycolor_vs";
						textures_channel_config.pshader = "scene_texture1stage_keycolor_ps";
						textures_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("wall.jpg", Texture())) };

						auto zdepth_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ZDepthChannel_Queue_Entity") };
						zdepth_channel_config.vshader = "scene_zdepth_vs";
						zdepth_channel_config.pshader = "scene_zdepth_ps";

						auto ambientlight_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_AmbientLightChannel_Queue_Entity") };
						ambientlight_channel_config.vshader = "scene_flatcolor_vs";
						ambientlight_channel_config.pshader = "scene_flatcolor_ps";

						auto lit_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_LitChannel_Queue_Entity") };
						lit_channel_config.vshader = "scene_lit_vs";
						lit_channel_config.pshader = "scene_lit_ps";

						auto em_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_EmissiveChannel_Queue_Entity") };
						em_channel_config.vshader = "scene_flatcolor_vs";
						em_channel_config.pshader = "scene_flatcolor_ps";


						helpers::PassesDescriptors passesDescriptors =
						{
							// config
							{
								{ "bufferRendering_Scene_TexturesChannel_Queue_Entity", textures_channel_config },
								{ "bufferRendering_Scene_ZDepthChannel_Queue_Entity", zdepth_channel_config },
								{ "bufferRendering_Scene_AmbientLightChannel_Queue_Entity", ambientlight_channel_config },
								{ "bufferRendering_Scene_LitChannel_Queue_Entity", lit_channel_config },
								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity", em_channel_config }
							},

							// vertex shader params
							{
							},

							// pixel shader params
							{

								{ "bufferRendering_Scene_AmbientLightChannel_Queue_Entity",
									{
										{ std::make_pair("std.ambientlight.color", "color") }
									}
								},
								{ "bufferRendering_Scene_LitChannel_Queue_Entity",
									{
										{ std::make_pair("std.light0.dir", "light_dir") }
									}
								},
								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity",
									{
										{ std::make_pair("std.black_color", "color") }
									}
								}
							}
						};

						renderingHelper->registerToPasses(m_entitygraph, m_wallEntity, passesDescriptors);
					}



					// sphere rendering
					{
						auto textures_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_TexturesChannel_Queue_Entity") };
						textures_channel_config.vshader = "scene_texture1stage_keycolor_vs";
						textures_channel_config.pshader = "scene_texture1stage_keycolor_ps";
						textures_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("marbre.jpg", Texture())) };

						auto zdepth_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ZDepthChannel_Queue_Entity") };
						zdepth_channel_config.vshader = "scene_zdepth_vs";
						zdepth_channel_config.pshader = "scene_zdepth_ps";
						
						auto ambientlight_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_AmbientLightChannel_Queue_Entity") };
						ambientlight_channel_config.vshader = "scene_flatcolor_vs";
						ambientlight_channel_config.pshader = "scene_flatcolor_ps";

						auto lit_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_LitChannel_Queue_Entity") };
						lit_channel_config.vshader = "scene_lit_vs";
						lit_channel_config.pshader = "scene_lit_ps";

						auto em_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_EmissiveChannel_Queue_Entity") };
						em_channel_config.vshader = "scene_flatcolor_vs";
						em_channel_config.pshader = "scene_flatcolor_ps";


						helpers::PassesDescriptors passesDescriptors =
						{
							// config
							{
								{ "bufferRendering_Scene_TexturesChannel_Queue_Entity", textures_channel_config },
								{ "bufferRendering_Scene_ZDepthChannel_Queue_Entity", zdepth_channel_config },
								{ "bufferRendering_Scene_AmbientLightChannel_Queue_Entity", ambientlight_channel_config },
								{ "bufferRendering_Scene_LitChannel_Queue_Entity", lit_channel_config },
								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity", em_channel_config }
							},

							// vertex shader params
							{
							},

							// pixel shader params
							{

								{ "bufferRendering_Scene_AmbientLightChannel_Queue_Entity",
									{
										{ std::make_pair("std.ambientlight.color", "color") }
									}
								},
								{ "bufferRendering_Scene_LitChannel_Queue_Entity",
									{
										{ std::make_pair("std.light0.dir", "light_dir") }
									}
								},
								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity",
									{
										{ std::make_pair("std.black_color", "color") }
									}
								}
							}
						};

						renderingHelper->registerToPasses(m_entitygraph, m_sphereEntity, passesDescriptors);
					}


					// tree rendering
					{
						auto textures_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_TexturesChannel_Queue_Entity") };
						textures_channel_config.vshader = "scene_texture1stage_keycolor_vs";
						textures_channel_config.pshader = "scene_texture1stage_keycolor_ps";
						textures_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("tree2_tex.bmp", Texture())) };
						textures_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						textures_channel_config.rs_list.at(0).setArg("none");

						auto zdepth_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ZDepthChannel_Queue_Entity") };
						zdepth_channel_config.vshader = "scene_zdepth_keycolor_vs";
						zdepth_channel_config.pshader = "scene_zdepth_keycolor_ps";
						zdepth_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("tree2_tex.bmp", Texture())) };
						zdepth_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						zdepth_channel_config.rs_list.at(0).setArg("none");

						auto ambientlight_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_AmbientLightChannel_Queue_Entity") };
						ambientlight_channel_config.vshader = "scene_flatcolor_keycolor_vs";
						ambientlight_channel_config.pshader = "scene_flatcolor_keycolor_ps";
						ambientlight_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("tree2_tex.bmp", Texture())) };
						ambientlight_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						ambientlight_channel_config.rs_list.at(0).setArg("none");

						auto lit_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_LitChannel_Queue_Entity") };
						lit_channel_config.vshader = "scene_lit_keycolor_vs";
						lit_channel_config.pshader = "scene_lit_keycolor_ps";
						lit_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("tree2_tex.bmp", Texture())) };
						lit_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						lit_channel_config.rs_list.at(0).setArg("none");


						auto em_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_EmissiveChannel_Queue_Entity") };
						em_channel_config.vshader = "scene_flatcolor_keycolor_vs";
						em_channel_config.pshader = "scene_flatcolor_keycolor_ps";
						em_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("tree2_tex.bmp", Texture())) };
						em_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						em_channel_config.rs_list.at(0).setArg("none");


						helpers::PassesDescriptors passesDescriptors =
						{
							// config
							{
								{ "bufferRendering_Scene_TexturesChannel_Queue_Entity", textures_channel_config },
								{ "bufferRendering_Scene_ZDepthChannel_Queue_Entity", zdepth_channel_config },
								{ "bufferRendering_Scene_AmbientLightChannel_Queue_Entity", ambientlight_channel_config },
								{ "bufferRendering_Scene_LitChannel_Queue_Entity", lit_channel_config },
								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity", em_channel_config }
							},

							// vertex shader params
							{
							},

							// pixel shader params
							{
								{ "bufferRendering_Scene_TexturesChannel_Queue_Entity",
									{
										{ std::make_pair("texture_keycolor_ps.key_color", "key_color") }
									}
								},
								{ "bufferRendering_Scene_ZDepthChannel_Queue_Entity",
									{
										{ std::make_pair("texture_keycolor_ps.key_color", "key_color") }
									}
								},
								{ "bufferRendering_Scene_AmbientLightChannel_Queue_Entity",
									{
										{ std::make_pair("texture_keycolor_ps.key_color", "key_color") },
										{ std::make_pair("std.ambientlight.color", "color") }
									}
								},
								{ "bufferRendering_Scene_LitChannel_Queue_Entity",
									{
										{ std::make_pair("texture_keycolor_ps.key_color", "key_color") },
										{ std::make_pair("std.light0.dir", "light_dir") }
									}
								},
								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity",
									{
										{ std::make_pair("texture_keycolor_ps.key_color", "key_color") },
										{ std::make_pair("std.black_color", "color") }
									}
								}
							}
						};

						renderingHelper->registerToPasses(m_entitygraph, m_treeEntity, passesDescriptors);
					}


					// clouds rendering
					{
						auto textures_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_TexturesChannel_Queue_Entity") };
						textures_channel_config.vshader = "scene_flatclouds_vs";
						textures_channel_config.pshader = "scene_flatclouds_ps";
						textures_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("flatclouds.jpg", Texture())) };
						textures_channel_config.rendering_order = 999;


						textures_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						textures_channel_config.rs_list.at(0).setArg("ccw");

						textures_channel_config.rs_list.at(1).setOperation(RenderState::Operation::ENABLEZBUFFER);
						textures_channel_config.rs_list.at(1).setArg("false");


						textures_channel_config.rs_list.at(3).setOperation(RenderState::Operation::SETTEXTUREFILTERTYPE);
						textures_channel_config.rs_list.at(3).setArg("linear_uvwrap");


						textures_channel_config.rs_list.at(4).setOperation(RenderState::Operation::ALPHABLENDENABLE);
						textures_channel_config.rs_list.at(4).setArg("true");

						RenderState rs_alphablendop(RenderState::Operation::ALPHABLENDOP, "add");
						RenderState rs_alphablendfunc(RenderState::Operation::ALPHABLENDFUNC, "always");
						RenderState rs_alphablenddest(RenderState::Operation::ALPHABLENDDEST, "invsrcalpha");
						RenderState rs_alphablendsrc(RenderState::Operation::ALPHABLENDSRC, "srcalpha");

						textures_channel_config.rs_list.push_back(rs_alphablendop);
						textures_channel_config.rs_list.push_back(rs_alphablendfunc);
						textures_channel_config.rs_list.push_back(rs_alphablenddest);
						textures_channel_config.rs_list.push_back(rs_alphablendsrc);


						auto em_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_EmissiveChannel_Queue_Entity") };
						em_channel_config.vshader = "scene_flatcolor_vs";
						em_channel_config.pshader = "scene_flatcolor_ps";
						em_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("tree2_tex.bmp", Texture())) };
						em_channel_config.rendering_order = 999;

						em_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						em_channel_config.rs_list.at(0).setArg("ccw");

						em_channel_config.rs_list.at(1).setOperation(RenderState::Operation::ENABLEZBUFFER);
						em_channel_config.rs_list.at(1).setArg("false");


						helpers::PassesDescriptors passesDescriptors =
						{
							// config
							{
								{ "bufferRendering_Scene_TexturesChannel_Queue_Entity", textures_channel_config },
								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity", em_channel_config }
							},


							// vertex shader params
							{
							},

							// pixel shader params
							{
								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity",
									{
										{ std::make_pair("skydome_emissive_color", "color") }
									}
								}
							}
						};

						renderingHelper->registerToPasses(m_entitygraph, m_cloudsEntity, passesDescriptors);
					}


					// skydome rendering
					{
						auto textures_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_TexturesChannel_Queue_Entity") };
						textures_channel_config.vshader = "scene_skydome_vs";
						textures_channel_config.pshader = "scene_skydome_ps";
						textures_channel_config.rendering_order = 900;

						textures_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						textures_channel_config.rs_list.at(0).setArg("ccw");

						textures_channel_config.rs_list.at(1).setOperation(RenderState::Operation::ENABLEZBUFFER);
						textures_channel_config.rs_list.at(1).setArg("false");


						textures_channel_config.rs_list.at(4).setOperation(RenderState::Operation::ALPHABLENDENABLE);
						textures_channel_config.rs_list.at(4).setArg("true");

						RenderState rs_alphablendop(RenderState::Operation::ALPHABLENDOP, "add");
						RenderState rs_alphablendfunc(RenderState::Operation::ALPHABLENDFUNC, "always");
						RenderState rs_alphablenddest(RenderState::Operation::ALPHABLENDDEST, "invsrcalpha");
						RenderState rs_alphablendsrc(RenderState::Operation::ALPHABLENDSRC, "srcalpha");

						textures_channel_config.rs_list.push_back(rs_alphablendop);
						textures_channel_config.rs_list.push_back(rs_alphablendfunc);
						textures_channel_config.rs_list.push_back(rs_alphablenddest);
						textures_channel_config.rs_list.push_back(rs_alphablendsrc);


						auto em_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_EmissiveChannel_Queue_Entity") };
						em_channel_config.vshader = "scene_flatcolor_vs";
						em_channel_config.pshader = "scene_flatcolor_ps";

						em_channel_config.rendering_order = 990;

						em_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						em_channel_config.rs_list.at(0).setArg("ccw");

						em_channel_config.rs_list.at(1).setOperation(RenderState::Operation::ENABLEZBUFFER);
						em_channel_config.rs_list.at(1).setArg("false");


						helpers::PassesDescriptors passesDescriptors =
						{
							// config
							{
								{ "bufferRendering_Scene_TexturesChannel_Queue_Entity", textures_channel_config },
								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity", em_channel_config }
							},

							// vertex shader params
							{
							},

							// pixel shader params
							{
								{ "bufferRendering_Scene_TexturesChannel_Queue_Entity",
									{
										{ std::make_pair("std.light0.dir", "light0_dir") },
										{ std::make_pair("scene_skydome_ps.atmo_scattering_flag_0", "atmo_scattering_flag_0") },
										{ std::make_pair("scene_skydome_ps.atmo_scattering_flag_1", "atmo_scattering_flag_1") },
										{ std::make_pair("scene_skydome_ps.atmo_scattering_flag_2", "atmo_scattering_flag_2") },
										{ std::make_pair("scene_skydome_ps.atmo_scattering_flag_3", "atmo_scattering_flag_3") },
										{ std::make_pair("scene_skydome_ps.atmo_scattering_flag_4", "atmo_scattering_flag_4") },
										{ std::make_pair("scene_skydome_ps.atmo_scattering_flag_5", "atmo_scattering_flag_5") },
									}
								},

								{ "bufferRendering_Scene_EmissiveChannel_Queue_Entity",
									{
										{ std::make_pair("skydome_emissive_color", "color") }
									}
								}
							}
						};

						renderingHelper->registerToPasses(m_entitygraph, m_skydomeEntity, passesDescriptors);
					}

					///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

					/////////////////////////////////////////////////////////////////////////////////////
					/////// set queue current cameras
		
					auto renderingQueueSystemInstance{ dynamic_cast<mage::RenderingQueueSystem*>(SystemEngine::getInstance()->getSystem(renderingQueueSystemSlot)) };

					renderingQueueSystemInstance->createViewGroup("player_camera");
					renderingQueueSystemInstance->setViewGroupMainView("player_camera", "camera_Entity");

					renderingQueueSystemInstance->addQueuesToViewGroup("player_camera",
					{ 
						"bufferRendering_Scene_TexturesChannel_Queue_Entity",
						"bufferRendering_Scene_AmbientLightChannel_Queue_Entity",
						"bufferRendering_Scene_EmissiveChannel_Queue_Entity",
						"bufferRendering_Scene_LitChannel_Queue_Entity",
						"bufferRendering_Scene_ZDepthChannel_Queue_Entity"
					});


					///////////////////////////////////////////////////////////////////////////////////////
					/////// setup shadows rendering

					////// I : create shadow map camera
					
					auto& lookatJointEntityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "shadowmap_lookatJoint_Entity") };

					const auto lookatJointEntity{ lookatJointEntityNode.data() };

					auto& lookat_time_aspect{ lookatJointEntity->makeAspect(core::timeAspect::id) };
					auto& lookat_world_aspect{ lookatJointEntity->makeAspect(core::worldAspect::id) };

					lookat_world_aspect.addComponent<transform::WorldPosition>("lookat_output");
					lookat_world_aspect.addComponent<core::maths::Real3Vector>("lookat_dest", core::maths::Real3Vector(0.0, skydomeInnerRadius + groundLevel, 0.0));
					lookat_world_aspect.addComponent<core::maths::Real3Vector>("lookat_localpos");

					lookat_world_aspect.addComponent<transform::Animator>("animator", transform::Animator(
							{
								{"lookatJointAnim.output", "lookat_output"},
								{"lookatJointAnim.dest", "lookat_dest"},
								{"lookatJointAnim.localpos", "lookat_localpos"},

							},
							helpers::makeLookatJointAnimator())
						);

					helpers::plugCamera(m_entitygraph, m_orthogonal_projection, "shadowmap_lookatJoint_Entity", "shadowmap_camera_Entity");

					m_shadowmap_joints_list.push_back("shadowmap_lookatJoint_Entity");

					/////// II : update rendering graph
					helpers::install_shadows_renderer_queues(m_entitygraph,
																w_width, w_height,
																characteristics_v_width, characteristics_v_height, 
																dataCloud->readDataValue<maths::Real4Vector>("shadowmap_resol")[0],													
																"bufferRendering_Scene_LitChannel_Queue_Entity",
																"bufferRendering_Combiner_Accumulate_Quad_Entity",
																"bufferRendering_Combiner_ModulateLitAndShadows",
																"bufferRendering_Scene_ShadowsChannel_Queue_Entity",
																"bufferRendering_Scene_ShadowMapChannel_Queue_Entity",
																"shadowMap_Texture_Entity"
																);

					/////// III : entities in rendering graph

					renderingHelper->registerPass("bufferRendering_Scene_ShadowsChannel_Queue_Entity");
					renderingHelper->registerPass("bufferRendering_Scene_ShadowMapChannel_Queue_Entity");


					auto& shadowMapNode{ m_entitygraph.node("shadowMap_Texture_Entity") };
					const auto shadowmap_texture_entity{ shadowMapNode.data() };
					auto& sm_resource_aspect{ shadowmap_texture_entity->aspectAccess(core::resourcesAspect::id) };
					std::pair<size_t, Texture>* sm_texture_ptr{ &sm_resource_aspect.getComponent<std::pair<size_t, Texture>>("standalone_rendering_target_texture")->getPurpose() };

					std::vector<helpers::ShadowSourceEntity> shadowSourceEntities;

					// ground shadows rendering
					{
						auto shadows_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ShadowsChannel_Queue_Entity") };
						shadows_channel_config.vshader = "scene_shadowsmask_vs";
						shadows_channel_config.pshader = "scene_shadowsmask_ps";
						shadows_channel_config.textures_ptr_list = { sm_texture_ptr };

						auto shadowmap_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ShadowMapChannel_Queue_Entity") };
						shadowmap_channel_config.vshader = "scene_zdepth_vs";
						shadowmap_channel_config.pshader = "scene_zdepth_ps";

						helpers::PassesDescriptors passesDescriptors =
						{
							// config
							{
								{ "bufferRendering_Scene_ShadowsChannel_Queue_Entity", shadows_channel_config },
								{ "bufferRendering_Scene_ShadowMapChannel_Queue_Entity", shadowmap_channel_config }
							},

							// vertex shader params
							{
							},

							// pixel shader params
							{

								{ 
									"bufferRendering_Scene_ShadowsChannel_Queue_Entity",
									{
										{ std::make_pair("shadow_bias", "shadow_bias") },
										{ std::make_pair("shadowmap_resol", "shadowmap_resol") }
									}
								}
							}
						};

						shadowSourceEntities.push_back({ m_groundEntity, passesDescriptors });
					}

					// wall shadows rendering
					{
						auto shadows_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ShadowsChannel_Queue_Entity") };
						shadows_channel_config.vshader = "scene_shadowsmask_vs";
						shadows_channel_config.pshader = "scene_shadowsmask_ps";
						shadows_channel_config.textures_ptr_list = { sm_texture_ptr };

						auto shadowmap_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ShadowMapChannel_Queue_Entity") };
						shadowmap_channel_config.vshader = "scene_zdepth_vs";
						shadowmap_channel_config.pshader = "scene_zdepth_ps";

						helpers::PassesDescriptors passesDescriptors =
						{
							// config
							{
								{ "bufferRendering_Scene_ShadowsChannel_Queue_Entity", shadows_channel_config },
								{ "bufferRendering_Scene_ShadowMapChannel_Queue_Entity", shadowmap_channel_config }
							},

							// vertex shader params
							{
							},

							// pixel shader params
							{
								{ "bufferRendering_Scene_ShadowsChannel_Queue_Entity",
									{
										{ std::make_pair("shadow_bias", "shadow_bias") },
										{ std::make_pair("shadowmap_resol", "shadowmap_resol") }
									}
								}
							}
						};

						shadowSourceEntities.push_back({ m_wallEntity, passesDescriptors });
					}


					// sphere shadows rendering
					{
						auto shadows_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ShadowsChannel_Queue_Entity") };
						shadows_channel_config.vshader = "scene_shadowsmask_vs";
						shadows_channel_config.pshader = "scene_shadowsmask_ps";
						shadows_channel_config.textures_ptr_list = { sm_texture_ptr };

						auto shadowmap_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ShadowMapChannel_Queue_Entity") };
						shadowmap_channel_config.vshader = "scene_zdepth_vs";
						shadowmap_channel_config.pshader = "scene_zdepth_ps";
						shadowmap_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						shadowmap_channel_config.rs_list.at(0).setArg("ccw");

						helpers::PassesDescriptors passesDescriptors =
						{
							// config
							{
								{ "bufferRendering_Scene_ShadowsChannel_Queue_Entity", shadows_channel_config },
								{ "bufferRendering_Scene_ShadowMapChannel_Queue_Entity", shadowmap_channel_config }
							},

							// vertex shader params
							{
							},

							// pixel shader params
							{
								{ "bufferRendering_Scene_ShadowsChannel_Queue_Entity",
									{
										{ std::make_pair("shadow_bias", "shadow_bias") },
										{ std::make_pair("shadowmap_resol", "shadowmap_resol") }
									}
								}
							}
						};

						shadowSourceEntities.push_back({ m_sphereEntity, passesDescriptors });
					}



					// tree shadows rendering
					{
						auto shadows_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ShadowsChannel_Queue_Entity") };
						shadows_channel_config.vshader = "scene_shadowsmask_keycolor_vs";
						shadows_channel_config.pshader = "scene_shadowsmask_keycolor_ps";
						shadows_channel_config.textures_ptr_list = { sm_texture_ptr };
						shadows_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_1, std::make_pair("tree2_tex.bmp", Texture())) };
						shadows_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						shadows_channel_config.rs_list.at(0).setArg("none");

						auto shadowmap_channel_config{ renderingHelper->getPassConfig("bufferRendering_Scene_ShadowMapChannel_Queue_Entity") };
						shadowmap_channel_config.vshader = "scene_zdepth_keycolor_vs";
						shadowmap_channel_config.pshader = "scene_zdepth_keycolor_ps";
						shadowmap_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("tree2_tex.bmp", Texture())) };
						shadowmap_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
						shadowmap_channel_config.rs_list.at(0).setArg("none");

						helpers::PassesDescriptors passesDescriptors =
						{
							// config
							{
								{ "bufferRendering_Scene_ShadowsChannel_Queue_Entity", shadows_channel_config },
								{ "bufferRendering_Scene_ShadowMapChannel_Queue_Entity", shadowmap_channel_config }
							},

							// vertex shader params
							{
							},

							// pixel shader params
							{
								{ "bufferRendering_Scene_ShadowsChannel_Queue_Entity",
									{
										{ std::make_pair("shadow_bias", "shadow_bias") },
										{ std::make_pair("shadowmap_resol", "shadowmap_resol") },
										{ std::make_pair("texture_keycolor_ps.key_color", "key_color") }
									}
								},
								{ "bufferRendering_Scene_ShadowMapChannel_Queue_Entity",
									{
										{ std::make_pair("texture_keycolor_ps.key_color", "key_color") }
									}
								}
							}
						};

						shadowSourceEntities.push_back({ m_treeEntity, passesDescriptors });						
					}

					for (const auto& shadowSourceEntity : shadowSourceEntities)
					{
						renderingHelper->registerToPasses(m_entitygraph, shadowSourceEntity.entity, shadowSourceEntity.passesDescriptors);
					}

					/////// IV : manage viewgroups

					renderingQueueSystemInstance->createViewGroup("player_camera_2");
					renderingQueueSystemInstance->setViewGroupMainView("player_camera_2", "camera_Entity");
					renderingQueueSystemInstance->setViewGroupSecondaryView("player_camera_2", "shadowmap_camera_Entity");
					
					renderingQueueSystemInstance->addQueuesToViewGroup("player_camera_2",
					{
						"bufferRendering_Scene_ShadowsChannel_Queue_Entity"
					});
					
					renderingQueueSystemInstance->createViewGroup("shadowmap_camera");
					renderingQueueSystemInstance->setViewGroupMainView("shadowmap_camera", "shadowmap_camera_Entity");
					renderingQueueSystemInstance->addQueuesToViewGroup("shadowmap_camera",
					{
						"bufferRendering_Scene_ShadowMapChannel_Queue_Entity"
					});
					
				}
				break;
			}
		}
	};
	d3d11System->registerSubscriber(d3d11_cb);
}


void SamplesOpenEnv::create_openenv_scenegraph(const std::string& p_parentEntityId)
{
	auto& appwindowNode{ m_entitygraph.node(p_parentEntityId) };
	const auto appwindow{ appwindowNode.data() };

	const auto& mainwindows_rendering_aspect{ appwindow->aspectAccess(mage::core::renderingAspect::id) };

	const float characteristics_v_width{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportWidth")->getPurpose() };
	const float characteristics_v_height{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportHeight")->getPurpose() };


	{
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "ground_Entity") };
		const auto entity{ entityNode.data() };

		auto& world_aspect{ entity->makeAspect(core::worldAspect::id) };
		entity->makeAspect(core::timeAspect::id);

		world_aspect.addComponent<transform::WorldPosition>("position");
		world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
		(
			{},
			[=](const core::ComponentContainer& p_world_aspect,
				const core::ComponentContainer& p_time_aspect,
				const transform::WorldPosition&,
				const std::unordered_map<std::string, std::string>&)
			{

				maths::Matrix positionmat;
				positionmat.translation(0.0, skydomeInnerRadius + groundLevel, 0.0);

				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = wp.local_pos * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("rect", "ground.ac"), TriangleMeshe()));

		m_groundEntity = entity;
	}


	////////////////////////////////

	{
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "wall_Entity") };
		const auto entity{ entityNode.data() };

		auto& world_aspect{ entity->makeAspect(core::worldAspect::id) };
		entity->makeAspect(core::timeAspect::id);

		world_aspect.addComponent<transform::WorldPosition>("position");
		world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
		(
			{},
			[=](const core::ComponentContainer& p_world_aspect,
				const core::ComponentContainer& p_time_aspect,
				const transform::WorldPosition&,
				const std::unordered_map<std::string, std::string>&)
			{

				maths::Matrix positionmat;
				positionmat.translation(-25.0, skydomeInnerRadius + groundLevel + 0.0, -40.0);

				maths::Matrix rotationmat;
				rotationmat.rotation(core::maths::Real3Vector(0, 1, 0), core::maths::degToRad(90));

				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = wp.local_pos * rotationmat * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };

		TriangleMeshe wall_triangle_meshe;
		wall_triangle_meshe.setSmoothNormalesGeneration(false);

		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("box", "wall.ac"), wall_triangle_meshe));

		m_wallEntity = entity;
	}





	////////////////////////////////

	{
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "sphere_Entity") };
		const auto entity{ entityNode.data() };

		auto& world_aspect{ entity->makeAspect(core::worldAspect::id) };
		entity->makeAspect(core::timeAspect::id);

		world_aspect.addComponent<transform::WorldPosition>("position");
		world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
		(
			{},
			[=](const core::ComponentContainer& p_world_aspect,
				const core::ComponentContainer& p_time_aspect,
				const transform::WorldPosition&,
				const std::unordered_map<std::string, std::string>&)
			{

				maths::Matrix positionmat;
				positionmat.translation(-45.0, skydomeInnerRadius + groundLevel + 8.0, -20.0);

				maths::Matrix rotationmat;
				rotationmat.rotation(core::maths::Real3Vector(0, 1, 0), core::maths::degToRad(135));


				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = wp.local_pos * rotationmat * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("sphere", "sphere.ac"), TriangleMeshe()));

		m_sphereEntity = entity;
	}


	////////////////////////////////

	{
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "clouds_Entity") };
		const auto entity{ entityNode.data() };

		auto& world_aspect{ entity->makeAspect(core::worldAspect::id) };
		entity->makeAspect(core::timeAspect::id);

		world_aspect.addComponent<transform::WorldPosition>("position");
		world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
		(
			{},
			[=](const core::ComponentContainer& p_world_aspect,
				const core::ComponentContainer& p_time_aspect,
				const transform::WorldPosition&,
				const std::unordered_map<std::string, std::string>&)
			{

				maths::Matrix positionmat;
				positionmat.translation(0.0, skydomeInnerRadius + groundLevel + 400, 0.0);

				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = wp.local_pos * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("rect", "flatclouds.ac"), TriangleMeshe()));

		m_cloudsEntity = entity;
	}

	////////////////////////////////

	{
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "tree_Entity") };
		const auto entity{ entityNode.data() };

		auto& world_aspect{ entity->makeAspect(core::worldAspect::id) };
		entity->makeAspect(core::timeAspect::id);

		world_aspect.addComponent<transform::WorldPosition>("position");
		world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
		(
			{},
			[=](const core::ComponentContainer& p_world_aspect,
				const core::ComponentContainer& p_time_aspect,
				const transform::WorldPosition&,
				const std::unordered_map<std::string, std::string>&)
			{

				maths::Matrix positionmat;
				positionmat.translation(0.0, skydomeInnerRadius + groundLevel, -30.0);

				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = wp.local_pos * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("Plane.001", "tree0.ac"), TriangleMeshe()));

		m_treeEntity = entity;
	}

	////////////////////////////////

	{
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "skydome_Entity") };
		const auto entity{ entityNode.data() };

		auto& world_aspect{ entity->makeAspect(core::worldAspect::id) };
		entity->makeAspect(core::timeAspect::id);

		world_aspect.addComponent<transform::WorldPosition>("position");
		world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
		(
			{},
			[=](const core::ComponentContainer& p_world_aspect,
				const core::ComponentContainer& p_time_aspect,
				const transform::WorldPosition&,
				const std::unordered_map<std::string, std::string>&)
			{

				maths::Matrix positionmat;
				positionmat.translation(0.0, 0.0, 0.0);

				maths::Matrix scalingmat;
				scalingmat.scale(skydomeOuterRadius, skydomeOuterRadius, skydomeOuterRadius);

				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = wp.local_pos * scalingmat * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("sphere", "skydome.ac"), TriangleMeshe()));

		m_skydomeEntity = entity;
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////// add projections

	m_perpective_projection.perspective(characteristics_v_width, characteristics_v_height, 1.0, 100000.00000000000);
	m_orthogonal_projection.orthogonal(characteristics_v_width * 400, characteristics_v_height * 400, 1.0, 100000.00000000000);

	/////////////// add camera with gimbal lock jointure ////////////////

	auto& gblJointEntityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "gblJoint_Entity") };

	const auto gblJointEntity{ gblJointEntityNode.data() };

	gblJointEntity->makeAspect(core::timeAspect::id);
	auto& gbl_world_aspect{ gblJointEntity->makeAspect(core::worldAspect::id) };

	gbl_world_aspect.addComponent<transform::WorldPosition>("gbl_output");

	gbl_world_aspect.addComponent<double>("gbl_theta", 0);
	gbl_world_aspect.addComponent<double>("gbl_phi", 0);
	gbl_world_aspect.addComponent<double>("gbl_speed", 0);
	gbl_world_aspect.addComponent<maths::Real3Vector>("gbl_pos", maths::Real3Vector(-50.0, skydomeInnerRadius + groundLevel + 5, 1.0));

	gbl_world_aspect.addComponent<transform::Animator>("animator", transform::Animator(
		{
			// input-output/components keys id mapping
			{"gimbalLockJointAnim.theta", "gbl_theta"},
			{"gimbalLockJointAnim.phi", "gbl_phi"},
			{"gimbalLockJointAnim.position", "gbl_pos"},
			{"gimbalLockJointAnim.speed", "gbl_speed"},
			{"gimbalLockJointAnim.output", "gbl_output"}

		}, helpers::makeGimbalLockJointAnimator()));


	// add camera

	helpers::plugCamera(m_entitygraph, m_perpective_projection, "gblJoint_Entity", "camera_Entity");

}

void SamplesOpenEnv::create_openenv_rendergraph(const std::string& p_parentEntityId, int p_w_width, int p_w_height, float p_characteristics_v_width, float p_characteristics_v_height)
{

	const auto combiner_fog_input_channnel{ Texture(Texture::Format::TEXTURE_RGB, p_w_width, p_w_height) };
	const auto combiner_fog_zdepths_channnel{ Texture(Texture::Format::TEXTURE_FLOAT32, p_w_width, p_w_height) };

	mage::helpers::plugRenderingQuad(m_entitygraph,
		"fog_queue",
		p_characteristics_v_width, p_characteristics_v_height,
		p_parentEntityId,
		"bufferRendering_Combiner_Fog_Queue_Entity",
		"bufferRendering_Combiner_Fog_Quad_Entity",
		"bufferRendering_Combiner_Fog_View_Entity",
		"combiner_fog_vs",
		"combiner_fog_ps",
		{
			std::make_pair(Texture::STAGE_0, combiner_fog_input_channnel),
			std::make_pair(Texture::STAGE_1, combiner_fog_zdepths_channnel),
		},
		Texture::STAGE_0);

	Entity* bufferRendering_Combiner_Fog_Quad_Entity{ m_entitygraph.node("bufferRendering_Combiner_Fog_Quad_Entity").data() };

	auto& screenRendering_Combiner_Fog_Quad_Entity_rendering_aspect{ bufferRendering_Combiner_Fog_Quad_Entity->aspectAccess(core::renderingAspect::id) };

	rendering::DrawingControl& fogDrawingControl{ screenRendering_Combiner_Fog_Quad_Entity_rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
	fogDrawingControl.pshaders_map.push_back(std::make_pair("std.fog.color", "fog_color"));
	fogDrawingControl.pshaders_map.push_back(std::make_pair("std.fog.density", "fog_density"));


	// channel : zdepth

	rendering::Queue zdepthChannelRenderingQueue("zdepth_channel_queue");
	zdepthChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
	zdepthChannelRenderingQueue.enableTargetClearing(true);
	zdepthChannelRenderingQueue.enableTargetDepthClearing(true);
	zdepthChannelRenderingQueue.setTargetStage(Texture::STAGE_1);

	mage::helpers::plugRenderingQueue(m_entitygraph, zdepthChannelRenderingQueue, "bufferRendering_Combiner_Fog_Quad_Entity", "bufferRendering_Scene_ZDepthChannel_Queue_Entity");


	const auto combiner_modulate_inputA_channnel{ Texture(Texture::Format::TEXTURE_RGB, p_w_width, p_w_height) };
	const auto combiner_modulate_inputB_channnel{ Texture(Texture::Format::TEXTURE_RGB, p_w_width, p_w_height) };

	mage::helpers::plugRenderingQuad(m_entitygraph,
		"modulate_queue",
		p_characteristics_v_width, p_characteristics_v_height,
		"bufferRendering_Combiner_Fog_Quad_Entity",
		"bufferRendering_Combiner_Modulate_Queue_Entity",
		"bufferRendering_Combiner_Modulate_Quad_Entity",
		"bufferRendering_Combiner_Modulate_View_Entity",
		"combiner_modulate_vs",
		"combiner_modulate_ps",
		{
			std::make_pair(Texture::STAGE_0, combiner_modulate_inputA_channnel),
			std::make_pair(Texture::STAGE_1, combiner_modulate_inputB_channnel),
		},
		Texture::STAGE_0);


	/////////////////////////////////////////////////////////////////

	// channel : textures

	rendering::Queue texturesChannelRenderingQueue("textures_channel_queue");
	texturesChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
	texturesChannelRenderingQueue.enableTargetClearing(true);
	texturesChannelRenderingQueue.enableTargetDepthClearing(true);
	texturesChannelRenderingQueue.setTargetStage(Texture::STAGE_1);

	mage::helpers::plugRenderingQueue(m_entitygraph, texturesChannelRenderingQueue, "bufferRendering_Combiner_Modulate_Quad_Entity", "bufferRendering_Scene_TexturesChannel_Queue_Entity");

	// channel : ligths and shadows effect

	const auto combiner_accumulate_inputA_channnel{ Texture(Texture::Format::TEXTURE_RGB, p_w_width, p_w_height) };
	const auto combiner_accumulate_inputB_channnel{ Texture(Texture::Format::TEXTURE_RGB, p_w_width, p_w_height) };
	const auto combiner_accumulate_inputC_channnel{ Texture(Texture::Format::TEXTURE_RGB, p_w_width, p_w_height) };


	mage::helpers::plugRenderingQuad(m_entitygraph,
		"acc_lit_queue",
		p_characteristics_v_width, p_characteristics_v_height,
		"bufferRendering_Combiner_Modulate_Quad_Entity",
		"bufferRendering_Combiner_Accumulate_Queue_Entity",
		"bufferRendering_Combiner_Accumulate_Quad_Entity",
		"bufferRendering_Combiner_Accumulate_View_Entity",

		"combiner_accumulate3chan_vs",
		"combiner_accumulate3chan_ps",
		{
			std::make_pair(Texture::STAGE_0, combiner_accumulate_inputA_channnel),
			std::make_pair(Texture::STAGE_1, combiner_accumulate_inputB_channnel),
			std::make_pair(Texture::STAGE_2, combiner_accumulate_inputC_channnel),
		},
		Texture::STAGE_0);


	// channel : ambient light

	rendering::Queue ambientLightChannelRenderingQueue("ambientlight_channel_queue");
	ambientLightChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
	ambientLightChannelRenderingQueue.enableTargetClearing(true);
	ambientLightChannelRenderingQueue.enableTargetDepthClearing(true);
	ambientLightChannelRenderingQueue.setTargetStage(Texture::STAGE_0);

	mage::helpers::plugRenderingQueue(m_entitygraph, ambientLightChannelRenderingQueue, "bufferRendering_Combiner_Accumulate_Quad_Entity", "bufferRendering_Scene_AmbientLightChannel_Queue_Entity");

	// channel : emissive

	rendering::Queue emissiveChannelRenderingQueue("emissive_channel_queue");
	emissiveChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
	emissiveChannelRenderingQueue.enableTargetClearing(true);
	emissiveChannelRenderingQueue.enableTargetDepthClearing(true);
	emissiveChannelRenderingQueue.setTargetStage(Texture::STAGE_1);

	mage::helpers::plugRenderingQueue(m_entitygraph, emissiveChannelRenderingQueue, "bufferRendering_Combiner_Accumulate_Quad_Entity", "bufferRendering_Scene_EmissiveChannel_Queue_Entity");

	// channel : directional lit

	rendering::Queue litChannelRenderingQueue("lit_channel_queue");
	litChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
	litChannelRenderingQueue.enableTargetClearing(true);
	litChannelRenderingQueue.enableTargetDepthClearing(true);
	litChannelRenderingQueue.setTargetStage(Texture::STAGE_2);

	mage::helpers::plugRenderingQueue(m_entitygraph, litChannelRenderingQueue, "bufferRendering_Combiner_Accumulate_Quad_Entity", "bufferRendering_Scene_LitChannel_Queue_Entity");
}
