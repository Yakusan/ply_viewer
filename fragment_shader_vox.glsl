uniform int flag;

varying vec3 norm;

void main(void)
{
    if (flag == 1)
        gl_FragColor = vec4(1. ,1. ,1. , 1.);
    else if (flag == 2)
        gl_FragColor = vec4(0. ,0. ,1. , 1.);
    else
        gl_FragColor = vec4(0., 0., 0., 1.);

}
