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


Texture2D txInputA           : register(t0);
SamplerState samInputA       : register(s0);

Texture2D txInputB           : register(t1);
SamplerState samInputB       : register(s1);

Texture2D txInputC           : register(t2);
SamplerState samInputC       : register(s2);


struct PS_INTPUT 
{
    float4 Position : SV_POSITION;
	float2 TexCoord0: TEXCOORD0;
};

#include "commons.hlsl"

float4 ps_main(PS_INTPUT input) : SV_Target
{
    float4 colorA = txInputA.Sample(samInputA, input.TexCoord0);
    float4 colorB = txInputB.Sample(samInputB, input.TexCoord0);
    float4 colorC = txInputC.Sample(samInputC, input.TexCoord0);
    
    float4 final_color = saturate(colorA + colorB + colorC);
    
    
    
    return final_color;
}
