#version 120

uniform float pointSize;
uniform mat4 mvpMatrix;

attribute vec4 vertex;
attribute float pointRowIndex;
attribute vec3 color;

varying float pointIdx;
varying vec3 vcolor;
varying vec3 vert;

void main() {
  gl_Position = mvpMatrix * vertex;
  gl_PointSize  = pointSize;

  // for use in fragment shader
  pointIdx = pointRowIndex;
  vcolor = color;
  vert = vertex.xyz;
}
