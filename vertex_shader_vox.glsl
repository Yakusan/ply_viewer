uniform mat4 mvpMatrix;
uniform float xt;
uniform float yt;
uniform float zt;

attribute vec3 vertex;
attribute vec3 norm;

varying vec3 v_norm;

void main(void)
{
   v_norm = normalize(norm);
   vertex += vec3(xt,yt,zt);
   gl_Position = mvpMatrix * vec4(vertex, 1.0);
}
