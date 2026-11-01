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

cbuffer constargs : register(b0)
{
    float4 vec[512];
    Matrix mat[512];
};

Texture2D shadowMap : register(t0);
SamplerState shadowMapSampler : register(s0);

struct PS_INTPUT 
{
    float4 Position : SV_POSITION;
    
    float2 TexCoord1 : TEXCOORD1;    //projected position from secondary view (shadow map scene)    
    float4 TexCoord2 : TEXCOORD2;   //position from secondary view (shadow map scene)
};

//#include "generic_rendering.hlsl"



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
        float shadow = 0.0;                               
        float shadowmap_depth = p_shadowMap.Sample(p_shadowMapSampler, shadowmap_texcoords/* + poissonDisk[index] / p_shadowmapResol*/);
            
        if (p_current_depth + 0.075 > shadowmap_depth)
        {
            shadow += 0.5;
        }
        
        mask_val = saturate(shadow);        
    }    
    return mask_val;
}

float4 ps_main(PS_INTPUT input) : SV_Target
{    
    float4 color;        
    int shadowMapResol = 4096; //vec[57].x;
    color.rgba = computeShadows(input.TexCoord2.z, input.TexCoord1, shadowMap, shadowMapSampler, shadowMapResol);
    return color;
}
