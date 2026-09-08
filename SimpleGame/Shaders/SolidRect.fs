#version 330 core
in vec3 v_Color;
in vec2 v_UV;
uniform sampler2D u_Font;
uniform bool u_HdrScene;
uniform float u_SceneIntensity;
layout(location=0) out vec4 FragColor;
void main()
{
    float coverage = v_UV.x < 0.0 ? 1.0 : texture(u_Font, v_UV).r;
    vec3 color = v_Color;
    if (u_HdrScene) {
        // The prototype's authored palette is sRGB. Store linear radiance in HDR.
        color = mix(color / 12.92, pow((max(color, vec3(0.0)) + 0.055) / 1.055, vec3(2.4)),
            step(vec3(0.04045), color)) * u_SceneIntensity;
    }
    FragColor = vec4(color, coverage);
}
