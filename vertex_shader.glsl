#version 120

uniform float pointSize;
uniform mat4 mvpMatrix;

attribute vec4 vertex;
attribute vec3 color;

varying vec3 vcolor;

void main() {
  gl_Position = mvpMatrix * vertex;
  gl_PointSize  = pointSize;

  vcolor = color;
}
