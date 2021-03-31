#version 120

uniform float pointsCount;
uniform float colorAxisMode;
uniform vec3 pointsBoundMin;
uniform vec3 pointsBoundMax;

varying vec3 vert;
varying vec3 vcolor;
varying float pointIdx;

void main() {
  gl_FragColor = vec4(vcolor, 1.);
}
