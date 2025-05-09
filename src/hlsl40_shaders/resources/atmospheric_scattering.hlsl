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


struct atmo_scattering_sampling_result
{
    float3 c0;
    float3 c1;
    float3 v3Direction;
};


// The scale equation calculated by Vernier's Graphical Analysis
float scale(float fCos)
{
    float fScaleDepth = 0.25;
    float x = 1.0 - fCos;
    return fScaleDepth * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

// Calculates the Rayleigh phase function
float getRayleighPhase(float fCos2)
{
	//return 1.0;
    return 0.75 + 0.75 * fCos2;
}

// Calculates the Mie phase function
float getMiePhase(float fCos, float fCos2, float g, float g2)
{
    return 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos2) / pow(1.0 + g2 - 2.0 * g * fCos, 1.5);
}

float3 atmo_scattering_color_result(atmo_scattering_sampling_result p_sampling, float3 p_ldir)
{
    float g = -0.990;
    float g2 = g * g;

    float3 color;

    float fCos = dot(p_ldir, p_sampling.v3Direction) / length(p_sampling.v3Direction);
    float fCos2 = fCos * fCos;
    color = getRayleighPhase(fCos2) * p_sampling.c0 + getMiePhase(fCos, fCos2, g, g2) * p_sampling.c1;

    return color;
}

atmo_scattering_sampling_result skyfromatmo_atmo_scattering_sampling(float3 p_vertex_pos, float3 p_camera_pos, float3 p_ldir)
{

    float4 atmo_scattering_flag_0 = vec[v_atmo_scattering_flag_0];
    float4 atmo_scattering_flag_1 = vec[v_atmo_scattering_flag_1];
    float4 atmo_scattering_flag_2 = vec[v_atmo_scattering_flag_2];
    float4 atmo_scattering_flag_3 = vec[v_atmo_scattering_flag_3];
    float4 atmo_scattering_flag_4 = vec[v_atmo_scattering_flag_4];
    float4 atmo_scattering_flag_5 = vec[v_atmo_scattering_flag_5];
    float4 atmo_scattering_flag_6 = vec[v_atmo_scattering_flag_6];

    atmo_scattering_sampling_result res;

    float3 v3CameraPos = p_camera_pos;

    ////////////////////////////////////////////////////

    // The number of sample points taken along the ray
    int nSamples = 2;
    float fSamples = (float) nSamples;

    float fScaleDepth = atmo_scattering_flag_1.x;
    float fInvScaleDepth = atmo_scattering_flag_1.y;

    float fOuterRadius = atmo_scattering_flag_0.x;
    float fInnerRadius = atmo_scattering_flag_0.y;
    float fOuterRadius2 = atmo_scattering_flag_0.z;
    float fInnerRadius2 = atmo_scattering_flag_0.w;

    float fCameraHeight = length(v3CameraPos);
    float fCameraHeight2 = fCameraHeight * fCameraHeight;

    float fScale = atmo_scattering_flag_1.z;
    float fScaleOverScaleDepth = atmo_scattering_flag_1.w;

    float3 v3InvWavelength;
    v3InvWavelength.xyz = atmo_scattering_flag_2.xyz;

    float fKr = atmo_scattering_flag_3.x;
    float fKm = atmo_scattering_flag_3.y;

    float fKr4PI = atmo_scattering_flag_3.z;
    float fKm4PI = atmo_scattering_flag_3.w;

    float ESun = atmo_scattering_flag_4.y;

    float fKrESun = fKr * ESun;
    float fKmESun = fKm * ESun;


    //////////////////////////

    float3 v3Pos = p_vertex_pos;

    float3 v3Ray = v3Pos - (v3CameraPos);
    float fFar = length(v3Ray);
    v3Ray /= fFar;

	// Calculate the ray's starting position, then calculate its scattering offset
    float3 v3Start = v3CameraPos;
    float fHeight = length(v3Start);
    float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fCameraHeight));
    float fStartAngle = dot(v3Ray, v3Start) / fHeight;
    float fStartOffset = fDepth * scale(fStartAngle);

	// Initialize the scattering loop variables
    float fSampleLength = fFar / fSamples;
    float fScaledLength = fSampleLength * fScale;
    float3 v3SampleRay = v3Ray * fSampleLength;
    float3 v3SamplePoint = v3Start + v3SampleRay * 0.5;

	// Now loop through the sample rays
    float3 v3FrontColor = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < nSamples; i++)
    {
        float fHeight = length(v3SamplePoint);
        float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
        float fLightAngle = dot(p_ldir, v3SamplePoint) / fHeight;
        float fCameraAngle = dot(v3Ray, v3SamplePoint) / fHeight;
        float fScatter = (fStartOffset + fDepth * (scale(fLightAngle) - scale(fCameraAngle)));
        float3 v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
        v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
        v3SamplePoint += v3SampleRay;
    }

	// Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel shader

    res.c0 = v3FrontColor * (v3InvWavelength * fKrESun);
    res.c1 = v3FrontColor * fKmESun;
    res.v3Direction = v3CameraPos - v3Pos;

    return res;
}

