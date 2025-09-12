
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

#include "streamedopenenv.h"
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
#include "scenestreamersystem.h"


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


void StreamedOpenEnv::init(const std::string p_appWindowsEntityName)
{
	Base::init(p_appWindowsEntityName);
	d3d11_system_events_openenv();
}


void StreamedOpenEnv::d3d11_system_events_openenv()
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


					m_perpective_projection.perspective(characteristics_v_width, characteristics_v_height, 1.0, 100000.00000000000);
					m_orthogonal_projection.orthogonal(characteristics_v_width * 400, characteristics_v_height * 400, 1.0, 100000.00000000000);



					//////////////////////////////////////////


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

					const auto skydomeOuterRadius{ dataCloud->readDataValue<double>("app.skydomeOuterRadius") };
					const auto skydomeInnerRadius{ dataCloud->readDataValue<double>("app.skydomeInnerRadius") };

					const auto skydomeSkyfromspace_ESun{ dataCloud->readDataValue<double>("app.skydomeSkyfromspace_ESun") };
					const auto skydomeSkyfromatmo_ESun{ dataCloud->readDataValue<double>("app.skydomeSkyfromatmo_ESun") };
					const auto skydomeGroundfromspace_ESun{ dataCloud->readDataValue<double>("app.skydomeGroundfromspace_ESun") };
					const auto skydomeGroundfromatmo_ESun{ dataCloud->readDataValue<double>("app.skydomeGroundfromatmo_ESun") };

					const auto skydomeWaveLength_x{ dataCloud->readDataValue<double>("app.skydomeWaveLength_x") };
					const auto skydomeWaveLength_y{ dataCloud->readDataValue<double>("app.skydomeWaveLength_y") };
					const auto skydomeWaveLength_z{ dataCloud->readDataValue<double>("app.skydomeWaveLength_z") };

					const auto skydomeKm{ dataCloud->readDataValue<double>("app.skydomeKm") };
					const auto skydomeKr{ dataCloud->readDataValue<double>("app.skydomeKr") };
					const auto skydomeScaleDepth{ dataCloud->readDataValue<double>("app.skydomeScaleDepth") };


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

					const char rendergraph_json[] = R"json(
					{
						"subs":
						[
							{
								"descr" : "Fog", 
								"shaders":
								[
									{ 
										"name" : "combiner_fog_vs",
										"args": 
										[
										]
									},
									{
										"name" : "combiner_fog_ps",
										"args":	
										[
											{ 
												"source" : "std.fog.color",
												"destination" : "fog_color"
											},
											{ 
												"source" : "std.fog.density",
												"destination" : "fog_density"
											}
										]
									}
								],
								"inputs":
								[
									{
										"stage" : 0,
										"buffer_texture" :
										{
											"format_descr" : "TEXTURE_RGB"
										}
									},
									{
										"stage" : 1,
										"buffer_texture" :
										{
											"format_descr" : "TEXTURE_FLOAT32"
										}
									}
								],
								"target_stage": 0,
								"subs":
								[
								]
							}
						]
					}
					)json";


					const char scenegraph_json[] = R"json(
					{
						"subs":
						[
							{
								"descr": "cameraGblJoint_Entity",

								"world_aspect" : 
								{
									"animators":
									[
										{
											"helper": "gimbalLockJoin"
										},
										{
											"helper": "matrixFactory",
											"matrix_factory":
											{
												"type": "translation",	
												"descr": "cameraGblJoint_Entity initial pos (part I)",

												"x_direct_value":
												{
													"descr" : "cameraGblJoint_Entity initial pos in x (part I)",
													"value": -50
												},

												"y_datacloud_value":
												{
													"descr" : "cameraGblJoint_Entity initial pos in y (part I)",
													"var_name": "app.skydomeInnerRadius"
												},

												"z_direct_value":
												{
													"descr" : "cameraGblJoint_Entity initial pos in z (part I)",
													"value": 1.5
												}
											}
										},
										{
											"helper": "matrixFactory",
											"matrix_factory":
											{
												"type": "translation",
												"descr": "cameraGblJoint_Entity initial pos (part II)",

												"x_direct_value":
												{
													"descr" : "cameraGblJoint_Entity initial pos in x (part II)",
													"value": 0
												},

												"y_datacloud_value":
												{
													"descr" : "cameraGblJoint_Entity initial pos in y (part II)",
													"var_name": "app.groundLevel"
												},

												"z_direct_value":
												{
													"descr" : "cameraGblJoint_Entity initial pos in z (part II)",
													"value": 0
												}
											}
										}
									]
								},

								"subs":
								[
									{
										"descr": "camera_Entity",
										"helper": "plugCamera"
									}
								]
							}
						]
					}
					)json";

					auto sceneStreamerSystemInstance{ dynamic_cast<mage::SceneStreamerSystem*>(SystemEngine::getInstance()->getSystem(sceneStreamSystemSlot)) };

					sceneStreamerSystemInstance->buildRendergraphPart(rendergraph_json, "screenRendering_Filter_DirectForward_Quad_Entity",
																		w_width, w_height, characteristics_v_width, characteristics_v_height);

					sceneStreamerSystemInstance->buildScenegraphPart(scenegraph_json, "app_Entity", m_perpective_projection);


				}
				break;
			}
		}
	};
	d3d11System->registerSubscriber(d3d11_cb);
}

