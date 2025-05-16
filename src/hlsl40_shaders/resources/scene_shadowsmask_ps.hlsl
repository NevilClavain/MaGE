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

Texture2D shadowMap : register(t0);
SamplerState shadowMapSampler : register(s0);

struct PS_INTPUT 
{
    float4 Position : SV_POSITION;
    
    float2 TexCoord0 : TEXCOORD0;    //position from secondary view (shadow map scene)
};

float4 ps_main(PS_INTPUT input) : SV_Target
{    
    float4 color;
   
    // compute shadow map text coord
    float2 shadowmap_texcoords;
    shadowmap_texcoords.x = saturate((input.TexCoord0.x + 1.0) / 2.0);
    shadowmap_texcoords.y = saturate(1.0 - (input.TexCoord0.y + 1.0) / 2.0);
    
    // get shadow map depth
    float depth = shadowMap.Sample(shadowMapSampler, shadowmap_texcoords);
           
    // TEMP
    color.r = shadowmap_texcoords.x;
    color.g = depth / 1000000;
    color.b = shadowmap_texcoords.y;
    color.a = 1.0;
    
    return color;
}
