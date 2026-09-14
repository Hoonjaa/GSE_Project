#version 330 core
in vec3 v_Color;
uniform bool u_HdrScene;
uniform float u_SceneIntensity;
layout(location = 0) out vec4 FragColor;

void main()
{
    vec3 color = v_Color;
    if (u_HdrScene)
    {
        color = mix(color / 12.92,
                    pow((max(color, vec3(0.0)) + 0.055) / 1.055, vec3(2.4)),
                    step(vec3(0.04045), color)) * u_SceneIntensity;
    }
    FragColor = vec4(color, 1.0);
}
