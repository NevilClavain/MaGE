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

Texture2D txDiffuse : register(t1);
SamplerState txDiffuseSampler : register(s1);

struct PS_INTPUT 
{
    float4 Position : SV_POSITION;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;    //projected position from secondary view (shadow map scene)
    float4 TexCoord2 : TEXCOORD2;   //position from secondary view (shadow map scene)
};

#include "generic_rendering.hlsl"

float4 ps_main(PS_INTPUT input) : SV_Target
{        
    float4 key_color = vec[58];
    float4 texture_color = txDiffuse.Sample(txDiffuseSampler, input.TexCoord0);
    
    if (texture_color.r == key_color.r && texture_color.g == key_color.g && texture_color.b == key_color.b)
    {
        clip(-1);
    }
    
    float4 color;    
    float bias = vec[56].x;
    int shadowMapResol = vec[57].x;
    color.rgba = computeShadows(input.TexCoord2.z, input.TexCoord1, shadowMap, shadowMapSampler, shadowMapResol);
    return color;
}
