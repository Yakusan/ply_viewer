#version 120

attribute vec2 vertex;
attribute vec2 texCoord;

varying vec2 tCoord;

void main(void)
{
    gl_Position = vertex;
    tCoord = texCoord;
}
