
/* -*-LIC_BEGIN-*- */
/*
*
* MaGE rendering framework
* Emmanuel Chaumont Copyright (c) 2013-2026
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

#include "resourcesystem.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include "logger_service.h"

#include "trianglemeshe.h"
#include "filesystem.h"

#include "matrix.h"

using namespace mage;
using namespace mage::core;

static mage::core::maths::Matrix convertFromAssimpMatrix(const aiMatrix4x4& p_in_mat)
{
	mage::core::maths::Matrix mat;
	mat(0, 0) = p_in_mat.a1;
	mat(0, 1) = p_in_mat.b1;
	mat(0, 2) = p_in_mat.c1;
	mat(0, 3) = p_in_mat.d1;
	mat(1, 0) = p_in_mat.a2;
	mat(1, 1) = p_in_mat.b2;
	mat(1, 2) = p_in_mat.c2;
	mat(1, 3) = p_in_mat.d2;
	mat(2, 0) = p_in_mat.a3;
	mat(2, 1) = p_in_mat.b3;
	mat(2, 2) = p_in_mat.c3;
	mat(2, 3) = p_in_mat.d3;
	mat(3, 0) = p_in_mat.a4;
	mat(3, 1) = p_in_mat.b4;
	mat(3, 2) = p_in_mat.c4;
	mat(3, 3) = p_in_mat.d4;

	return mat;
}

void ResourceSystem::handleSceneFile(const std::string& p_filename, const std::string& p_mesheid, TriangleMeshe& p_mesheInfos, const core::ComponentList<std::map<std::string, SceneNode>>& p_nodes_hierarchy_list)
{
	_MAGE_DEBUG(m_localLogger, std::string("Handle scene ") + p_filename);

	const std::string mesheAction{ "load_meshe" };

	const std::string targetAction{ p_mesheid + "@" + p_filename };

	const auto task{ new mage::core::SimpleAsyncTask<>(mesheAction, targetAction,
		[&,
			mesheAction = mesheAction,
			currentIndex = m_runnerIndex,
			filename = p_filename,
			meshe_id = p_mesheid
		]()
		{
			_MAGE_DEBUG(m_localLoggerRunner, std::string("loading meshe ") + filename);

			// build full path
			const auto meshe_path{ m_meshesBasePath + "/" + filename };

			try
			{
				auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };

				p_mesheInfos.m_source = TriangleMeshe::Source::CONTENT_FROM_FILE;
				p_mesheInfos.m_source_id = filename;

				_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_MESHE_LOAD_BEGIN : " + filename);
				for (const auto& call : m_callbacks)
				{
					call(ResourceSystemEvent::RESOURCE_MESHE_LOAD_BEGIN, filename);
				}

				mage::core::FileContent<const char> meshe_text(meshe_path);
				meshe_text.load();

				const auto importer{ new Assimp::Importer() };

				auto flags{ aiProcess_Triangulate |
									aiProcess_JoinIdenticalVertices |
									aiProcess_FlipUVs |
									aiProcess_SortByPType };

				if (p_mesheInfos.hasSmoothNormalesGeneration())
				{
					flags |= aiProcess_GenSmoothNormals;
				}
				else
				{
					flags |= aiProcess_GenNormals;
				}

				const aiScene* scene{ importer->ReadFileFromMemory(meshe_text.getData(), meshe_text.getDataSize(), flags)};
				if (scene)
				{
					_MAGE_TRACE(m_localLoggerRunner, std::string("************************************SCENE INFOS BEGIN***********************************"));
					_MAGE_TRACE(m_localLoggerRunner, "file = " + meshe_path);

					_MAGE_TRACE(m_localLoggerRunner, "scene HasMeshes " + std::to_string(scene->HasMeshes()));
					_MAGE_TRACE(m_localLoggerRunner, "scene mNumMeshes " + std::to_string(scene->mNumMeshes));

					_MAGE_TRACE(m_localLoggerRunner, "scene HasTextures " + std::to_string(scene->HasTextures()));
					_MAGE_TRACE(m_localLoggerRunner, "scene mNumTextures " + std::to_string(scene->mNumTextures));

					_MAGE_TRACE(m_localLoggerRunner, "scene HasMaterials " + std::to_string(scene->HasMaterials()));
					_MAGE_TRACE(m_localLoggerRunner, "scene mNumMaterials " + std::to_string(scene->mNumMaterials));

					_MAGE_TRACE(m_localLoggerRunner, "scene HasLights " + std::to_string(scene->HasLights()));
					_MAGE_TRACE(m_localLoggerRunner, "scene mNumLights " + std::to_string(scene->mNumLights));

					_MAGE_TRACE(m_localLoggerRunner, "scene HasCameras " + std::to_string(scene->HasCameras()));
					_MAGE_TRACE(m_localLoggerRunner, "scene mNumCameras " + std::to_string(scene->mNumCameras));

					_MAGE_TRACE(m_localLoggerRunner, "scene HasAnimations " + std::to_string(scene->HasAnimations()));
					_MAGE_TRACE(m_localLoggerRunner, "scene mNumAnimations " + std::to_string(scene->mNumAnimations));

					_MAGE_TRACE(m_localLoggerRunner, std::string("************************************SCENE INFOS END***********************************"));

					const auto root{ scene->mRootNode };

					_MAGE_TRACE(m_localLoggerRunner, std::string("************************************NODE HIERARCHY BEGIN***********************************"));

					const std::function<void(aiNode*, int)> dumpAssimpSceneNode
					{
						[&](aiNode* p_ai_node, int depth)
						{
							std::string spacing(depth, ' ');
							_MAGE_TRACE(m_localLoggerRunner, spacing + std::string("node : ") + p_ai_node->mName.C_Str() + std::string(" nb children : ") + std::to_string(p_ai_node->mNumChildren));
							_MAGE_TRACE(m_localLoggerRunner, spacing + std::string("nb meshes : ") + std::to_string(p_ai_node->mNumMeshes));

							_MAGE_TRACE(m_localLoggerRunner, spacing + std::string("  -> ") << p_ai_node->mTransformation.a1 << " " << p_ai_node->mTransformation.b1 << " " << p_ai_node->mTransformation.c1 << " " << p_ai_node->mTransformation.d1)
							_MAGE_TRACE(m_localLoggerRunner, spacing + std::string("  -> ") << p_ai_node->mTransformation.a2 << " " << p_ai_node->mTransformation.b2 << " " << p_ai_node->mTransformation.c2 << " " << p_ai_node->mTransformation.d2)
							_MAGE_TRACE(m_localLoggerRunner, spacing + std::string("  -> ") << p_ai_node->mTransformation.a3 << " " << p_ai_node->mTransformation.b3 << " " << p_ai_node->mTransformation.c3 << " " << p_ai_node->mTransformation.d3)
							_MAGE_TRACE(m_localLoggerRunner, spacing + std::string("  -> ") << p_ai_node->mTransformation.a4 << " " << p_ai_node->mTransformation.b4 << " " << p_ai_node->mTransformation.c4 << " " << p_ai_node->mTransformation.d4)


							for (size_t i = 0; i < p_ai_node->mNumChildren; i++)
							{
								dumpAssimpSceneNode(p_ai_node->mChildren[i], depth + 1);
							}
						}
					};

					dumpAssimpSceneNode(root, 1);

					_MAGE_TRACE(m_localLoggerRunner, std::string("************************************NODE HIERARCHY END***********************************"));


					//// record scene nodes hierarchy
					std::map<std::string, SceneNode> scene_nodes;

					const std::function<void(aiNode*)> recordAssimpSceneNode
					{
						[&](aiNode* p_ai_node)
						{
							SceneNode node;
							node.id = p_ai_node->mName.C_Str();
							if (p_ai_node->mParent)
							{
								node.parent_id = p_ai_node->mParent->mName.C_Str();
							}

							node.locale_transform = convertFromAssimpMatrix(p_ai_node->mTransformation);
							for (size_t i = 0; i < p_ai_node->mNumChildren; i++)
							{
								node.children.push_back(p_ai_node->mChildren[i]->mName.C_Str());
								recordAssimpSceneNode(p_ai_node->mChildren[i]);
							}

							scene_nodes.emplace(node.id, node);
						}
					};

					recordAssimpSceneNode(root);
					p_mesheInfos.setSceneNodes(scene_nodes, root->mName.C_Str());


					/////////////////////////////////// Meshe animations

					for (size_t i = 0; i < scene->mNumAnimations; i++)
					{
						//_DSTRACE((*rs_logger), dsstring("Animation ") << i);

						_MAGE_TRACE(m_localLoggerRunner, std::string("Animation : ") + std::to_string(i));

						const auto animation{ scene->mAnimations[i] };

						_MAGE_TRACE(m_localLoggerRunner, std::string("	Name = ") + animation->mName.C_Str());
						_MAGE_TRACE(m_localLoggerRunner, std::string("	TicksPerSeconds = ") + std::to_string(animation->mTicksPerSecond));
						_MAGE_TRACE(m_localLoggerRunner, std::string("	Duration (ticks) = ") + std::to_string(animation->mDuration));
						_MAGE_TRACE(m_localLoggerRunner, std::string("	Num Channels = ") + std::to_string(animation->mNumChannels));

						/////////////////////////////////////////

						aiAnimation* ai_animation{ scene->mAnimations[i] };
						AnimationKeys animation_keys;

						animation_keys.duration_ticks = ai_animation->mDuration;
						animation_keys.ticks_per_seconds = ai_animation->mTicksPerSecond;
						animation_keys.name = ai_animation->mName.C_Str();

						for (size_t j = 0; j < ai_animation->mNumChannels; j++)
						{
							aiNodeAnim* ai_node_anim{ ai_animation->mChannels[j] };
							NodeAnimation node_animation;

							node_animation.node_name = ai_node_anim->mNodeName.C_Str();

							for (size_t k = 0; k < ai_node_anim->mNumPositionKeys; k++)
							{
								aiVectorKey ai_key = ai_node_anim->mPositionKeys[k];
								VectorKey pos_key{ ai_key.mTime, { ai_key.mValue[0], ai_key.mValue[1], ai_key.mValue[2], 1.0 } };

								node_animation.position_keys.push_back(pos_key);
							}

							for (size_t k = 0; k < ai_node_anim->mNumScalingKeys; k++)
							{
								aiVectorKey ai_key = ai_node_anim->mScalingKeys[k];
								VectorKey scaling_key{ ai_key.mTime, { ai_key.mValue[0], ai_key.mValue[1], ai_key.mValue[2], 1.0 } };

								node_animation.scaling_keys.push_back(scaling_key);
							}

							for (size_t k = 0; k < ai_node_anim->mNumRotationKeys; k++)
							{
								aiQuatKey ai_key = ai_node_anim->mRotationKeys[k];
								QuaternionKey quat_key{ ai_key.mTime, { ai_key.mValue.x, ai_key.mValue.y, ai_key.mValue.z, ai_key.mValue.w } };

								node_animation.rotations_keys.push_back(quat_key);
							}

							animation_keys.channels.emplace(node_animation.node_name, node_animation);
						}

						p_mesheInfos.push(animation_keys);
					}

					/////////////////////////////////////////////////////

					const auto meshe_node{ root->FindNode(meshe_id.c_str()) };
					const auto meshes{ scene->mMeshes };

					if (!meshe_node)
					{
						_EXCEPTION(std::string("cannot locate meshe id inside the .ac file : ") + meshe_id);
					}

					const auto nb_meshes{ meshe_node->mNumMeshes };


					_MAGE_TRACE(m_localLoggerRunner, std::string("************************************MESHE INFOS BEGIN***********************************"));

					const auto name{ meshe_node->mName.C_Str() };
					_MAGE_TRACE(m_localLoggerRunner, std::string("owner node = ") + name);
					_MAGE_TRACE(m_localLoggerRunner, std::string("nb_meshes = ") << nb_meshes);

					p_mesheInfos.clearAnimationBones();
					const auto indexes{ meshe_node->mMeshes };
					for (unsigned int i = 0; i < nb_meshes; i++)
					{
						const auto meshe{ meshes[indexes[i]] };

						_MAGE_TRACE(m_localLoggerRunner, std::string(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>MESHE ") << i);
						_MAGE_TRACE(m_localLoggerRunner, std::string("name = ") << meshe->mName.C_Str());
						_MAGE_TRACE(m_localLoggerRunner, std::string("meshe HasPositions ") << meshe->HasPositions());
						_MAGE_TRACE(m_localLoggerRunner, std::string("meshe HasFaces ") << meshe->HasFaces());
						_MAGE_TRACE(m_localLoggerRunner, std::string("meshe HasNormals ") << meshe->HasNormals());
						_MAGE_TRACE(m_localLoggerRunner, std::string("meshe HasTangentsAndBitangents ") << meshe->HasTangentsAndBitangents());
						_MAGE_TRACE(m_localLoggerRunner, std::string("meshe NumUVChannels ") << meshe->GetNumUVChannels());
						_MAGE_TRACE(m_localLoggerRunner, std::string("meshe HasBones ") << meshe->HasBones());
						_MAGE_TRACE(m_localLoggerRunner, std::string("meshe NumBones ") << meshe->mNumBones);
						_MAGE_TRACE(m_localLoggerRunner, std::string("meshe NumFaces ") << meshe->mNumFaces);
						_MAGE_TRACE(m_localLoggerRunner, std::string("meshe NumVertices ") << meshe->mNumVertices);

						for (size_t j = 0; j < meshe->mNumBones; j++)
						{
							const auto bone{ meshe->mBones[j] };

							_MAGE_TRACE(m_localLoggerRunner, std::string("Bone ") << j);
							_MAGE_TRACE(m_localLoggerRunner, std::string("  -> name = ") << bone->mName.C_Str());
							_MAGE_TRACE(m_localLoggerRunner, std::string("  -> offsetMatrx"));

							_MAGE_TRACE(m_localLoggerRunner, std::string("  -> ") << bone->mOffsetMatrix.a1 << " " << bone->mOffsetMatrix.b1 << " " << bone->mOffsetMatrix.c1 << " " << bone->mOffsetMatrix.d1);
							_MAGE_TRACE(m_localLoggerRunner, std::string("  -> ") << bone->mOffsetMatrix.a2 << " " << bone->mOffsetMatrix.b2 << " " << bone->mOffsetMatrix.c2 << " " << bone->mOffsetMatrix.d2);
							_MAGE_TRACE(m_localLoggerRunner, std::string("  -> ") << bone->mOffsetMatrix.a3 << " " << bone->mOffsetMatrix.b3 << " " << bone->mOffsetMatrix.c3 << " " << bone->mOffsetMatrix.d3);
							_MAGE_TRACE(m_localLoggerRunner, std::string("  -> ") << bone->mOffsetMatrix.a4 << " " << bone->mOffsetMatrix.b4 << " " << bone->mOffsetMatrix.c4 << " " << bone->mOffsetMatrix.d4);

							_MAGE_TRACE(m_localLoggerRunner, std::string("  -> weights"));

							/*
							for (size_t k = 0; k < bone->mNumWeights; k++)
							{
								_MAGE_TRACE(m_localLoggerRunner, std::string("  -> vertex ") << bone->mWeights[k].mVertexId << " weight " << bone->mWeights[k].mWeight );
							}
							*/
						}
					}
					_MAGE_TRACE(m_localLoggerRunner, std::string("************************************MESHE INFOS END***********************************"));


					p_mesheInfos.clearTriangles();
					p_mesheInfos.clearVertices();

					int global_index = 0;
					for (unsigned int i = 0; i < nb_meshes; i++)
					{
						const auto meshe{ meshes[indexes[i]] };

						for (size_t j = 0; j < meshe->mNumFaces; j++)
						{
							const auto face{ meshe->mFaces[j] };

							if (face.mNumIndices != 3)
							{
								_EXCEPTION("Face must have exactly 3 indices");
							}

							const auto i1{ face.mIndices[0] };
							const auto i2{ face.mIndices[1] };
							const auto i3{ face.mIndices[2] };

							const TrianglePrimitive<unsigned int> t{ i1 + global_index, i2 + global_index, i3 + global_index };
							p_mesheInfos.push(t);
						}

						const aiVector3D zero3D(0.0f, 0.0f, 0.0f);

						for (size_t j = 0; j < meshe->mNumVertices; j++)
						{
							const auto v_in{ meshe->mVertices[j] };

							Vertex v_out(v_in[0], v_in[1], v_in[2]);

							if (meshe->HasBones())
							{
								v_out.tu[4] = -1.0;
								v_out.tv[4] = -1.0;
								v_out.tw[4] = -1.0;
								v_out.ta[4] = -1.0;
								v_out.tu[5] = 0.0;
								v_out.tv[5] = 0.0;
								v_out.tw[5] = 0.0;
								v_out.ta[5] = 0.0;

								v_out.tu[6] = -1.0;
								v_out.tv[6] = -1.0;
								v_out.tw[6] = -1.0;
								v_out.ta[6] = -1.0;
								v_out.tu[7] = 0.0;
								v_out.tv[7] = 0.0;
								v_out.tw[7] = 0.0;
								v_out.ta[7] = 0.0;
							}

							if (meshe->GetNumUVChannels() > 0)
							{
								const auto texCoord{ meshe->HasTextureCoords(0) ? meshe->mTextureCoords[0][j] : zero3D };
								v_out.tu[0] = texCoord[0];
								v_out.tv[0] = texCoord[1];
							}

							// model has its own normales, so use it
							v_out.nx = meshe->mNormals[j][0];
							v_out.ny = meshe->mNormals[j][1];
							v_out.nz = meshe->mNormals[j][2];

							p_mesheInfos.push(v_out);
						}
					}

					for (unsigned int i = 0; i < nb_meshes; i++)
					{
						const auto meshe{ meshes[indexes[i]] };
						for (size_t j = 0; j < meshe->mNumBones; j++)
						{
							const auto bone{ meshe->mBones[j] };

							AnimationBone bone_output;
							bone_output.offset_matrix = convertFromAssimpMatrix(bone->mOffsetMatrix);
							p_mesheInfos.push(bone_output, std::string(bone->mName.C_Str()));

							for (size_t k = 0; k < bone->mNumWeights; k++)
							{
								const auto weight{ bone->mWeights[k].mWeight };
								const auto vert_index{ bone->mWeights[k].mVertexId };
								auto vertex{ p_mesheInfos.getVertex(vert_index) };

								if (vertex.tu[4] == -1.0)
								{
									vertex.tu[4] = j;       // j = bone index
									vertex.tu[5] = weight;
								}
								else if (vertex.tv[4] == -1.0)
								{
									vertex.tv[4] = j;       // j = bone index
									vertex.tv[5] = weight;

								}
								else if (vertex.tw[4] == -1.0)
								{
									vertex.tw[4] = j;       // j = bone index
									vertex.tw[5] = weight;
								}
								else if (vertex.ta[4] == -1.0)
								{
									vertex.ta[4] = j;       // j = bone index
									vertex.ta[5] = weight;
								}

								else if (vertex.tu[6] == -1.0)
								{
									vertex.tu[6] = j;       // j = bone index
									vertex.tu[7] = weight;
								}
								else if (vertex.tv[6] == -1.0)
								{
									vertex.tv[6] = j;       // j = bone index
									vertex.tv[7] = weight;
								}
								else if (vertex.tw[6] == -1.0)
								{
									vertex.tw[6] = j;       // j = bone index
									vertex.tw[7] = weight;
								}
								else if (vertex.ta[6] == -1.0)
								{
									vertex.ta[6] = j;       // j = bone index
									vertex.ta[7] = weight;
								}

								else
								{
									_EXCEPTION("A vertex cannot reference more than 8 bones");
									//_MAGE_WARN(m_localLoggerRunner, "A vertex cannot reference more than 8 bones, ignored. bone " + std::string(bone->mName.C_Str()));
								}

								p_mesheInfos.update(vert_index, vertex);
							}
						}
					}
				}
				else
				{
					_EXCEPTION(std::string("No scene in file : ") + filename);
				}
				delete importer;

				p_mesheInfos.computeSize();
				p_mesheInfos.computeResourceUID();

				_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_MESHE_LOAD_SUCCESS : " + filename);
				for (const auto& call : m_callbacks)
				{
					call(ResourceSystemEvent::RESOURCE_MESHE_LOAD_SUCCESS, filename);
				}

				p_mesheInfos.setState(TriangleMeshe::State::BLOBLOADED);
			}
			catch (const std::exception& e)
			{
				_MAGE_ERROR(m_localLoggerRunner, std::string("failed to manage ") + meshe_path + " : reason = " + e.what());

				// send error status to main thread and let terminate
				const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, filename, mesheAction };
				m_runner[currentIndex].get()->m_mailbox_out.push(report);
			}
		}
	) };
	
	m_runner[m_runnerIndex].get()->m_mailbox_in.push(task);

	m_runnerIndex++;
	if (m_runnerIndex == nbRunners)
	{
		m_runnerIndex = 0;
	}

}