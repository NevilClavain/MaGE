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



Texture2D txInputA           : register(t0);
SamplerState samInputA       : register(s0);

Texture2D txInputB           : register(t1);
SamplerState samInputB       : register(s1);


struct PS_INTPUT 
{
    float4 Position : SV_POSITION;
	float2 TexCoord0: TEXCOORD0;
};

#include "commons.hlsl"

float4 PS_BlurH(float2 texelSize, float2 uv) : SV_TARGET
{
    float4 result = 0.0;

    result += txInputB.Sample(samInputB, uv + float2(-4, 0) * texelSize).r * 0.01;
    result += txInputB.Sample(samInputB, uv + float2(-3, 0) * texelSize).r * 0.02;
    result += txInputB.Sample(samInputB, uv + float2(-2, 0) * texelSize).r * 0.05;
    result += txInputB.Sample(samInputB, uv + float2(-1, 0) * texelSize).r * 0.25;
    result += txInputB.Sample(samInputB, uv).r * 0.40;
    result += txInputB.Sample(samInputB, uv + float2(1, 0) * texelSize).r * 0.25;
    result += txInputB.Sample(samInputB, uv + float2(2, 0) * texelSize).r * 0.05;
    result += txInputB.Sample(samInputB, uv + float2(3, 0) * texelSize).r * 0.02;
    result += txInputB.Sample(samInputB, uv + float2(4, 0) * texelSize).r * 0.01;

    return result;
}

float4 PS_BlurV(float2 texelSize, float2 uv) : SV_TARGET
{
    float4 result = 0.0;

    result += txInputB.Sample(samInputB, uv + float2(0, -4) * texelSize).r * 0.01;
    result += txInputB.Sample(samInputB, uv + float2(0, -3) * texelSize).r * 0.02;
    result += txInputB.Sample(samInputB, uv + float2(0, -2) * texelSize).r * 0.05;
    result += txInputB.Sample(samInputB, uv + float2(0, -1) * texelSize).r * 0.25;
    result += txInputB.Sample(samInputB, uv).r * 0.40;
    result += txInputB.Sample(samInputB, uv + float2(0, 1) * texelSize).r * 0.25;
    result += txInputB.Sample(samInputB, uv + float2(0, 2) * texelSize).r * 0.05;
    result += txInputB.Sample(samInputB, uv + float2(0, 3) * texelSize).r * 0.02;
    result += txInputB.Sample(samInputB, uv + float2(0, 4) * texelSize).r * 0.01;

    return result;
}

float4 ps_main(PS_INTPUT input) : SV_Target
{
    float4 colorA = txInputA.Sample(samInputA, input.TexCoord0);


    uint width, height;
    txInputB.GetDimensions(width, height);

    float2 texelSize;
    texelSize.x = 1.0f / width;
    texelSize.y = 1.0f / height;

    float4 colorShadowsH = PS_BlurH(texelSize, input.TexCoord0);
    float4 colorShadowsV = PS_BlurV(texelSize, input.TexCoord0);
    float4 colorShadows = ((0.5 * colorShadowsH) + (0.5 * colorShadowsV));

    float4 final_color = colorA * colorShadows;

    return final_color;
}
