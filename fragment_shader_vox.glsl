uniform int flag;

void main(void)
{
    if (flag)
        gl_FragColor = vec4(1., 1., 1., 1.);
    else
        gl_FragColor = vec4(0., 0., 0., 1.);

}