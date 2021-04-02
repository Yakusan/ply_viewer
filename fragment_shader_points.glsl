#version 120

varying vec3 vert;
varying vec3 vcolor;
varying float pointIdx;

void main() {
  gl_FragColor = vec4(vcolor, 1.);
}
