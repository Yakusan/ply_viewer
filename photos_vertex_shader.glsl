#version 120

attribute vec2 vertex;
attribute vec2 texCoord;

varying vec2 tCoord;

void main(void)
{
    gl_Position = vec4(vertex, 0.0, 1.0);
    tCoord = texCoord;
}
