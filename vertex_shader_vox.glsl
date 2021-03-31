uniform mat4 mvpMatrix;
uniform float xt;
uniform float yt;
uniform float zt;

attribute vec3 vertex;

void main(void)
{
   vertex += vec3(xt,yt,zt);
   gl_Position = mvpMatrix * vec4(vertex, 1.0);
}
