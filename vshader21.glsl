#version 130

uniform mat4 modelview;
uniform mat4 projection;
uniform vec4 eyeposition;
uniform mat4 shadowmodel_light1;
uniform mat4 shadowmodel_light2;
uniform mat4 shadowmodel_light3;
uniform mat4 shadowproj;
uniform mat4 depthbiasmatrix;

uniform vec4 light1_pos;
uniform vec4 light2_pos;
uniform vec4 light3_pos;

in vec4 vPosition;
in vec4 vNormal;
in vec4 checkerBoardCoord;

out vec3 fE1;
out vec3 fE2;
out vec3 fE3;
out vec3 fN;
out vec3 fL1;
out vec3 fL2;
out vec3 fL3;
out vec4 ShadowCoord_light1;
out vec4 ShadowCoord_light2;
out vec4 ShadowCoord_light3;
out vec4 checkerBoardCoordPass;

void
main()
{
    gl_Position = projection * modelview * vPosition;

    fN = vNormal.xyz;
    fE1 = (eyeposition - vPosition).xyz;
    fE2 = (eyeposition - vPosition).xyz;
    fE3 = (eyeposition - vPosition).xyz;

    if ( light1_pos.w != 0.0 ) fL1 = light1_pos.xyz - vPosition.xyz;
    else fL1 = light1_pos.xyz;
    
    if ( light2_pos.w != 0.0 ) fL2 = light2_pos.xyz - vPosition.xyz;
    else fL2 = light2_pos.xyz;

    if ( light3_pos.w != 0.0 ) fL3 = light3_pos.xyz - vPosition.xyz;
    else fL3 = light3_pos.xyz;
    
    ShadowCoord_light1 = depthbiasmatrix * shadowproj * shadowmodel_light1 * vPosition;
    ShadowCoord_light2 = depthbiasmatrix * shadowproj * shadowmodel_light2 * vPosition;
    ShadowCoord_light3 = depthbiasmatrix * shadowproj * shadowmodel_light3 * vPosition;
    checkerBoardCoordPass = checkerBoardCoord;
}
