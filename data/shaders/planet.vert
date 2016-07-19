/*
 * Stellarium
 * Copyright (C) 2002-2016 Fabien Chereau and Stellarium contributors
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

/*
  This is the vertex shader for solar system object rendering
 */

attribute highp vec3 vertex; // vertex projected by CPU
attribute mediump vec2 texCoord;
attribute highp vec3 unprojectedVertex; //original vertex coordinate
#ifdef IS_OBJ
    attribute mediump vec3 normalIn; // OBJs have pre-calculated normals
#endif

uniform highp mat4 projectionMatrix;

varying mediump vec2 texc; //texture coord
varying highp vec3 P; //original unprojected position

#ifdef IS_OBJ
    //OBJ uses single normal for oren-nayar
    varying mediump vec3 normal;
#else
    #ifdef IS_MOON
        //Luna uses normal mapping
        varying highp vec3 normalX;
        varying highp vec3 normalY;
        varying highp vec3 normalZ;
    #else
        //normal objects use gourard shading
        //good enough for our spheres
        uniform highp vec3 lightDirection;
        varying mediump float lum_;
    #endif
#endif

void main()
{
    gl_Position = projectionMatrix * vec4(vertex, 1.);
    texc = texCoord;
    P = unprojectedVertex;
    
#ifdef IS_OBJ
    //OBJ uses imported normals
    normal = normalIn;
#else
    //other objects use the spherical normals
    highp vec3 normal = normalize(unprojectedVertex);
    #ifdef IS_MOON
        normalX = normalize(cross(vec3(0,0,1), normal));
        normalY = normalize(cross(normal, normalX));
        normalZ = normal;
    #else
        //simple Lambert illumination
        mediump float c = dot(lightDirection, normal);
        lum_ = clamp(c, 0.0, 1.0);
    #endif
#endif
}
