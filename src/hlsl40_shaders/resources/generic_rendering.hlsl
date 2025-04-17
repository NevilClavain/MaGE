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