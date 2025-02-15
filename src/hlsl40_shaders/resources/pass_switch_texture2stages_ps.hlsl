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


Texture2D txDiffuse0         : register(t0);
SamplerState sam0            : register(s0);

Texture2D txDiffuse1         : register(t1);
SamplerState sam1            : register(s1);


struct PS_INTPUT 
{
    float4 Position : SV_POSITION;
	float2 TexCoord0: TEXCOORD0;
};


float4 ps_main(PS_INTPUT input) : SV_Target
{
    float4 color_0;
    float4 color_1;
    
    color_0.xyz = txDiffuse0.Sample(sam0, input.TexCoord0);
    color_0.w = 1.0f;
    
    color_1.xyz = txDiffuse1.Sample(sam1, input.TexCoord0);
    color_1.w = 1.0f;
        
    return color_0;
    //return color_1;
}
