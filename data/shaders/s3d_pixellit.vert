/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2014 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
 
/*
This is a shader for phong/per-pixel lighting.
*/
 
#version 120

//macros that can be set by ShaderManager (simple true/false flags)
#define SHADOWS 0
#define BUMP 0
#define HEIGHT 0

//matrices
uniform mat4 u_mModelView;
uniform mat4 u_mProjection;
uniform mat3 u_mNormal;

uniform vec3 u_vLightDirectionView; //in view space, from point to light

#if SHADOWS
//shadow transforms
uniform mat4 u_mShadow0;
uniform mat4 u_mShadow1;
uniform mat4 u_mShadow2;
uniform mat4 u_mShadow3;
#endif

attribute vec4 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_texcoord;
#if BUMP
attribute vec4 a_tangent;
#endif

varying vec3 v_normal; //normal in view space
varying vec2 v_texcoord;
varying vec3 v_lightVec; //light vector, in VIEW or TBN space according to bump settings
varying vec3 v_viewPos; //position of fragment in view space

#if SHADOWS

varying vec4 v_shadowCoord0;
varying vec4 v_shadowCoord1;
varying vec4 v_shadowCoord2;
varying vec4 v_shadowCoord3;
#endif


void main(void)
{
	//transform normal
	v_normal = normalize(u_mNormal * a_normal);
	
	//pass on tex coord
	v_texcoord = a_texcoord;
	
	//calc vertex pos in view space
	vec4 viewPos = u_mModelView * a_vertex;
	v_viewPos = viewPos.xyz;
	
	#if SHADOWS
	//calculate shadowmap coords
	v_shadowCoord0 = u_mShadow0 * a_vertex;
	v_shadowCoord1 = u_mShadow1 * a_vertex;
	v_shadowCoord2 = u_mShadow2 * a_vertex;
	v_shadowCoord3 = u_mShadow3 * a_vertex;
	#endif
	
	#if BUMP
	//create View-->TBN matrix
	vec3 t = normalize(u_mNormal * a_tangent.xyz);
	//bitangent recreated from normal and tangent instead passed as attribute for a bit more orthonormality
	vec3 b = cross(v_normal, t) * a_tangent.w; //w coordinate stores handedness of tangent space
	
	mat3 TBN = mat3(t.x, b.x, v_normal.x,
					t.y, b.y, v_normal.y,
					t.z, b.z, v_normal.z);
	v_lightVec = TBN * u_vLightDirectionView;
	v_viewPos = TBN * v_viewPos;
	#else
	v_lightVec = normalize(u_vLightDirectionView);
	#endif
	
	//calc final position
	gl_Position = u_mProjection * viewPos;
}