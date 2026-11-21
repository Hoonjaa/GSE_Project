#version 330 core
in vec3 v_Color;
in vec2 v_UV;
uniform sampler2D u_Font;
uniform sampler2D u_Panel;
uniform sampler2D u_Compass;
uniform bool u_HdrScene;
uniform float u_SceneIntensity;
uniform float u_SceneLight;
layout(location = 0) out vec4 FragColor;

void main()
{
    // Overlay sprites share the same ordered batch as the font and solid geometry.
    if (v_UV.x >= 4.0)
    {
        FragColor = texture(u_Compass, v_UV - vec2(4.0, 0.0));
        return;
    }
    if (v_UV.x >= 2.0)
    {
        FragColor = texture(u_Panel, v_UV - vec2(2.0, 0.0));
        return;
    }
    float coverage = v_UV.x < 0.0 ? (v_UV.y < 0.0 ? 1.0 : v_UV.y) : texture(u_Font, v_UV).r;
    vec3 color = v_Color;
    if (u_HdrScene)
    {
        // The prototype's authored palette is sRGB. Store linear radiance in HDR.
        color = mix(color / 12.92,
                    pow((max(color, vec3(0.0)) + 0.055) / 1.055, vec3(2.4)),
                    step(vec3(0.04045), color)) *
                u_SceneIntensity;
    }
    color *= u_SceneLight;
    FragColor = vec4(color, coverage);
}
