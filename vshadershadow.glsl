#version 130

uniform mat4 shadowmodel;
uniform mat4 shadowproj;

//layout(location=0) in vec4 vVertex;
in vec4 vVertex;

void
main()
{
    gl_Position = shadowproj * shadowmodel * vVertex;
}
