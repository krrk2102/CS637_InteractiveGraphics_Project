#version 130

uniform bool obj;
uniform vec4 light1_diffuse_product, light1_ambient_product, light1_specular_product;
uniform vec4 light2_diffuse_product, light2_ambient_product, light2_specular_product;
uniform vec4 light3_diffuse_product, light3_ambient_product, light3_specular_product;
uniform float shininess;
uniform sampler2D shadowMap_light1;
uniform sampler2D shadowMap_light2;
uniform sampler2D shadowMap_light3;
uniform sampler2D checkerBoard;

in vec3 fE1;
in vec3 fE2;
in vec3 fE3;
in vec3 fN;
in vec3 fL1;
in vec3 fL2;
in vec3 fL3;
in vec4 ShadowCoord_light1;
in vec4 ShadowCoord_light2;
in vec4 ShadowCoord_light3;
in vec4 checkerBoardCoordPass;

void
main()
{
    vec2 poissonDist[4] = vec2[](
        vec2( -0.94201624, -0.39906216 ),
        vec2( 0.94558609, -0.76890725 ),
        vec2( -0.094184101, -0.92938870 ),
        vec2( 0.34495938, 0.29387760 )
    );
    float shadow_brightness1 = 1, shadow_brightness2 = 1, shadow_brightness3 = 1;
    vec4 checkerBoardColor;

    vec3 newCoord_light1 = ShadowCoord_light1.xyz/ShadowCoord_light1.w;
    vec3 newCoord_light2 = ShadowCoord_light2.xyz/ShadowCoord_light2.w;
    vec3 newCoord_light3 = ShadowCoord_light3.xyz/ShadowCoord_light3.w;

    for (int i = 0; i < 4; i++) {
        if (texture2D(shadowMap_light1, newCoord_light1.xy + poissonDist[i]/700.0).z < newCoord_light1.z) 
            shadow_brightness1 -= 0.24;
        if (texture2D(shadowMap_light2, newCoord_light2.xy + poissonDist[i]/700.0).z < newCoord_light2.z) 
            shadow_brightness2 -= 0.24;
        if (texture2D(shadowMap_light3, newCoord_light3.xy + poissonDist[i]/700.0).z < newCoord_light3.z) 
            shadow_brightness3 -= 0.24;
    }
    if (!obj) checkerBoardColor = texture2D(checkerBoard, checkerBoardCoordPass.xy);
    if (newCoord_light1.x<0 || newCoord_light1.x>1 || newCoord_light1.y<0 || newCoord_light1.y>1)
        shadow_brightness1 = 1;
    if (newCoord_light2.x<0 || newCoord_light2.x>1 || newCoord_light2.y<0 || newCoord_light2.y>1)
        shadow_brightness2 = 1;
    if (newCoord_light3.x<0 || newCoord_light3.x>1 || newCoord_light3.y<0 || newCoord_light3.y>1)
        shadow_brightness3 = 1;

    vec4 diffuse1, specular1, diffuse2, specular2, diffuse3, specular3;
    vec3 E1 = normalize( fE1 );
    vec3 E2 = normalize( fE2 );
    vec3 E3 = normalize( fE3 );
    vec3 N1 = normalize( fN );
    vec3 N2 = normalize( fN );
    vec3 N3 = normalize( fN );
    vec3 L1 = normalize( fL1 );
    vec3 L2 = normalize( fL2 );
    vec3 L3 = normalize( fL3 );
    vec3 H1 = normalize( L1 + E1 );
    vec3 H2 = normalize( L2 + E2 );
    vec3 H3 = normalize( L3 + E3 );

    float Kd1 = max( dot( L1, N1 ), 0 );
    if (obj) diffuse1 = shadow_brightness1 * Kd1 * light1_diffuse_product;
    else diffuse1 = 0.1 * checkerBoardColor + shadow_brightness1 * Kd1 * light1_diffuse_product;
    
    float Kd2 = max( dot( L2, N2 ), 0 );
    if (obj) diffuse2 = shadow_brightness2 * Kd2 * light2_diffuse_product;
    else diffuse2 = 0.1 * checkerBoardColor + shadow_brightness2 * Kd2 * light2_diffuse_product;

    float Kd3 = max( dot( L3, N3 ), 0 );
    if (obj) diffuse3 = shadow_brightness3 * Kd3 * light3_diffuse_product;
    else diffuse3 = 0.1 * checkerBoardColor + shadow_brightness3 * Kd3 * light3_diffuse_product;

    float Ks1 = pow( max( dot( N1, H1 ), 0 ), shininess );
    specular1 = shadow_brightness1 * Ks1 * light1_specular_product;
    if ( dot( L1, N1 ) < 0.0 ) specular1 = vec4( 0.0, 0.0, 0.0, 1.0 );
    
    float Ks2 = pow( max( dot( N2, H2 ), 0 ), shininess );
    specular2 = shadow_brightness2 * Ks2 * light2_specular_product;
    if ( dot( L2, N2 ) < 0.0 ) specular2 = vec4( 0.0, 0.0, 0.0, 1.0 );

    float Ks3 = pow( max( dot( N3, H3 ), 0 ), shininess );
    specular3 = shadow_brightness3 * Ks3 * light3_specular_product;
    if ( dot( L3, N3 ) < 0.0 ) specular3 = vec4( 0.0, 0.0, 0.0, 1.0 );

    gl_FragColor = vec4( ( diffuse1 + specular1 + light1_ambient_product + diffuse2+ specular2 + light2_ambient_product + diffuse3 + specular3 + light3_ambient_product ).xyz, 1 );
}
