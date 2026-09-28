#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec2 a_Metadata;
layout(location = 3) in vec3 i_Offset;
layout(location = 4) in vec2 i_Rotation;
layout(location = 5) in vec3 i_Scale;
layout(location = 6) in vec2 i_Animation;
uniform vec2 u_ProjectionScale;
out vec3 v_Color;

void main()
{
    vec3 local = a_Position;
    vec3 color = a_Color;
    int mode = int(i_Animation.x + 0.5);
    if (mode == 1)
    {
        float phase = i_Animation.y * 0.8 + a_Metadata.x;
        float light = 0.5 + 0.5 * sin(phase);
        local.x += sin(phase) * 0.35 + a_Metadata.y * light * 0.7;
        color += light * vec3(0.055, 0.08, 0.07);
    }
    else if (mode == 2)
    {
        local.z += sin(i_Animation.y * 3.0) * 0.15 * a_Metadata.y;
    }
    else if (mode == 3)
    {
        local.y += sin(i_Animation.y) * 0.16 * a_Metadata.y;
    }
    local *= i_Scale;
    vec3 p = vec3(
        local.x * i_Rotation.x - local.z * i_Rotation.y,
        local.y,
        local.x * i_Rotation.y + local.z * i_Rotation.x) + i_Offset;
    gl_Position = vec4(
        (p.x - p.z) * 0.70710678 * u_ProjectionScale.x,
        (p.y * 0.81649658 - (p.x + p.z) * 0.40824829) * u_ProjectionScale.y,
        -(p.x + p.y + p.z) * 0.57735027 / 2048.0, 1.0);
    v_Color = color;
}
