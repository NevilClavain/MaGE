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

cbuffer constargs : register(b0)
{
    float4 vec[512];
    Matrix mat[512];
};

#define v_light_dir                56
#define v_key_color                57

Texture2D txDiffuse     : register(t0);
SamplerState sam        : register(s0);


struct PS_INTPUT 
{
    float4 Position     : SV_POSITION;
    float2 TexCoord0    : TEXCOORD0;
    float4 Normale      : TEXCOORD1;
};

#include "mat_input_constants.hlsl"
#include "generic_rendering.hlsl"

float4 ps_main(PS_INTPUT input) : SV_Target
{
    float4 key_color = vec[v_key_color];
    
    float4 texture_color = txDiffuse.Sample(sam, input.TexCoord0);    
    if (texture_color.r == key_color.r && texture_color.g == key_color.g && texture_color.b == key_color.b)
    {
        clip(-1);
    }
               
    float4x4 mat_World = mat[matWorld];
    
    float4 light_dir_global;
    light_dir_global = vec[v_light_dir];
    
    float4 color = {0, 0, 0, 1};
    
    const float4 object_normale = input.Normale;        
    const float3 world_object_normale = transformedNormaleForLights(object_normale, mat_World); 
           
    color.rgb += computePixelColorFromDirectionalLight(light_dir_global.xyz, world_object_normale);
        
    color.a = 1.0;
    return color;
}
