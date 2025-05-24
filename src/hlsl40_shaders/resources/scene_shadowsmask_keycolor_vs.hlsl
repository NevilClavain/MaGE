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

#include "mat_input_constants.hlsl"

struct VS_INPUT 
{
    float3 Position : POSITION;
    float4 TexCoord0 : TEXCOORD0;
};

struct VS_OUTPUT 
{
    float4 Position : SV_POSITION;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
    float4 TexCoord2 : TEXCOORD2;
};

VS_OUTPUT vs_main( VS_INPUT Input )
{
    VS_OUTPUT Output;
    float4 pos;
    pos.xyz = Input.Position;    
    pos.w = 1.0;
    
    // compute vertex position from main view (shadows mask scene) 
    float4 projected_pos_mainview = mul(pos, mat[matWorldViewProjection]);
    
    // compute vertex position from secondary view (shadow map scene)    
    float4 projected_pos_secondaryview = mul(pos, mat[matWorldViewProjectionSecondary]);
    
    Output.Position = projected_pos_mainview;
    Output.TexCoord0 = Input.TexCoord0.xy;
    
    
    Output.TexCoord1 = projected_pos_secondaryview.xy;
    
    float4 wv = mul(pos, mat[matWorldViewSecondary]);
    Output.TexCoord2 = wv;
    
    
    return( Output );   
}
