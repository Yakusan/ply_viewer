#version 120

uniform sampler2D texColor;

varying vec2 tCoord;

void main(void)
{
    gl_FragColor = vec4(texture2D(texColor, tCoord).rgb, 0.5);
}
