#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
uniform vec3 u_Offset;
uniform vec2 u_Rotation;
uniform vec2 u_ProjectionScale;
out vec3 v_Color;

void main()
{
    vec3 p = vec3(
        a_Position.x * u_Rotation.x - a_Position.z * u_Rotation.y,
        a_Position.y,
        a_Position.x * u_Rotation.y + a_Position.z * u_Rotation.x) + u_Offset;
    gl_Position = vec4(
        (p.x - p.z) * 0.70710678 * u_ProjectionScale.x,
        (p.y * 0.81649658 - (p.x + p.z) * 0.40824829) * u_ProjectionScale.y,
        -(p.x + p.y + p.z) * 0.57735027 / 2048.0,
        1.0);
    v_Color = a_Color;
}
