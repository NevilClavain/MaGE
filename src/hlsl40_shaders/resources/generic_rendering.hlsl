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

//pour lumieres diffuses : NORMAL transformee (sans les translations)
//dans repereworld

float4 transformedNormaleForLights(float4 p_normale, float4x4 p_worldmat)
{
    float4x4 worldRot = p_worldmat;
    worldRot[0][3] = 0.0;
    worldRot[1][3] = 0.0;
    worldRot[2][3] = 0.0;
    
    return mul(worldRot, p_normale); //mul(p_normale, worldRot);
}

float3 computePixelColorFromDirectionalLight(float3 p_light_dir, float3 p_world_object_normale)
{
    float3 normalized_world_normale = normalize(p_world_object_normale);        
    float diff = dot(normalize(-p_light_dir), normalized_world_normale);
    
    float3 litpixel_color;    
    
    litpixel_color.xyz = max(0.0, diff);

    return litpixel_color;
}

float randomForShadows(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

// compute shadow mask value with help of shadow map
float computeShadows(float p_current_depth, float2 p_projected_pos_secondaryview, Texture2D p_shadowMap, SamplerState p_shadowMapSampler, int p_shadowmapResol)
{
    float mask_val = 1.0;
    
    // compute text coord in shadow map, from projected_pos_secondaryview
    float2 shadowmap_texcoords;    
    shadowmap_texcoords.x = (p_projected_pos_secondaryview.x + 1.0) / 2.0;
    shadowmap_texcoords.y = 1.0 - (p_projected_pos_secondaryview.y + 1.0) / 2.0;
    
    if (shadowmap_texcoords.x >= 0.0 && shadowmap_texcoords.x <= 1.0 && shadowmap_texcoords.y >= 0.0 && shadowmap_texcoords.y <= 1.0)
    {
        static const float2 poissonDisk[4] =
        {
            float2(-0.94201624, -0.39906216),
            float2(0.94558609, -0.76890725),
            float2(-0.094184101, -0.92938870),
            float2(0.34495938, 0.29387760)
        };
        
        float shadow = 0.0;
        
        for (int i = 0; i < 4; i++)
        {
            int index = 4 * randomForShadows(float2(i, i + 1));
            
            float shadowmap_depth = p_shadowMap.Sample(p_shadowMapSampler, shadowmap_texcoords + poissonDisk[index] / p_shadowmapResol);
            
            if (p_current_depth > shadowmap_depth)
            {
                shadow += 0.2;
            }
        }
        mask_val = saturate(shadow);
    }
    
    return mask_val;
}