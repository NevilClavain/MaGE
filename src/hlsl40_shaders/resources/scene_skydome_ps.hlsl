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

#define v_light0_dir                56

#define v_atmo_scattering_flag_0    57
#define v_atmo_scattering_flag_1    58
#define v_atmo_scattering_flag_2    59
#define v_atmo_scattering_flag_3    60
#define v_atmo_scattering_flag_4    61
#define v_atmo_scattering_flag_5    62
#define v_atmo_scattering_flag_6    63

struct PS_INTPUT 
{
    float4 Position     : SV_POSITION;
    float4 t0           : TEXCOORD0;
    float4 t1           : TEXCOORD1;
};


#include "atmospheric_scattering.hlsl"

float4 compute_from_surface(float4 p_t0, float4 p_t1, float3 p_ldir)
{
    float4 color;
   
    atmo_scattering_sampling_result sampling_down = skyfromatmo_atmo_scattering_sampling(p_t0.xyz, p_t1.xyz, p_ldir);
    float3 color_down = atmo_scattering_color_result(sampling_down, p_ldir);

    color.xyz = color_down;
    color.w = 1.0;

    return color;
}

float4 ps_main(PS_INTPUT input) : SV_Target
{       
    float4 color = 0.0;
    
    float3 light0_dir_global;
    light0_dir_global.xyz = vec[v_light0_dir].xyz;
    
    float3 norm_light0_dir_global = -normalize(light0_dir_global);    
    float alpha;
    if (norm_light0_dir_global.y < 0.0)
    {
        alpha = 1.0 + norm_light0_dir_global.y;
    }
    else
    {
        alpha = 1.0;
    }
    
    const float mag_factor_max = 20.0;
    float mag_factor;
    
    const float limit = 0.08;
    
    /*
    if (norm_light0_dir_global.y < limit)
    {
        mag_factor = 1.0;

    }
    else
    {
        mag_factor = lerp(1.0, mag_factor_max, clamp(norm_light0_dir_global.y - limit, 0.0, 1.0));
    }
    */
    
    mag_factor = 1.0;
    
    color += mag_factor * compute_from_surface(input.t0, input.t1, norm_light0_dir_global);
    
    
    float4 final_color;
    final_color.w = alpha;

    // added S. O'Neil HDR correction
    final_color.xyz = 1.0 * (1.0 - exp(-0.91 * color.xyz));
    
    return final_color;    
 }
