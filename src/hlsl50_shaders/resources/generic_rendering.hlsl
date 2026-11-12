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

//pour lumieres diffuses : NORMAL transformee (sans les translations)
//dans repereworld

float4 transformedNormaleForLights(float4 p_normale, float4x4 p_worldmat)
{    
    float4x4 worldRot = p_worldmat;
    worldRot[3][0] = 0.0;
    worldRot[3][1] = 0.0;
    worldRot[3][2] = 0.0;
    
    return mul(p_normale, worldRot);    
}

float3 computePixelColorFromDirectionalLight(float3 p_light_dir, float3 p_world_object_normale)
{
    float3 normalized_world_normale = normalize(p_world_object_normale);        
    float diff = dot(normalize(-p_light_dir), normalized_world_normale);
    
    float3 litpixel_color;    
    
    litpixel_color.xyz = max(0.0, diff);

    return litpixel_color;
}


// compute shadow mask value with help of shadow map
float computeShadows(float p_bias, float p_shadow_attenuation, float p_current_depth, float3 p_projected_pos_secondaryview, Texture2D p_shadowMap, SamplerState p_shadowMapSampler)
{
    // compute text coord in shadow map, from projected_pos_secondaryview
    float2 shadowmap_texcoords;
    shadowmap_texcoords.x = (p_projected_pos_secondaryview.x + 1.0) / 2.0;
    shadowmap_texcoords.y = 1.0 - (p_projected_pos_secondaryview.y + 1.0) / 2.0;
    
    if (shadowmap_texcoords.y > 1.0 || shadowmap_texcoords.y < 0.0)
    {
        return 1.0;
    }

    if (shadowmap_texcoords.x > 1.0 || shadowmap_texcoords.x < 0.0)
    {
        return 1.0;
    }
        
    float shadow = p_shadow_attenuation;
    float shadowmap_depth = p_shadowMap.Sample(p_shadowMapSampler, shadowmap_texcoords);
                
    if (p_current_depth + p_bias > shadowmap_depth)
    {
        shadow = 1.0;
    }
    return shadow;
}