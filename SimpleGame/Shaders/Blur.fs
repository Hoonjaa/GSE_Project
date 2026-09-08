#version 330 core
in vec2 v_UV;
uniform sampler2D u_Source;
uniform vec2 u_Step;
layout(location=0) out vec4 FragColor;
void main()
{
    const float weight[5] = float[5](0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);
    vec3 color = texture(u_Source, v_UV).rgb * weight[0];
    for (int i = 1; i < 5; ++i) {
        color += texture(u_Source, v_UV + u_Step * float(i)).rgb * weight[i];
        color += texture(u_Source, v_UV - u_Step * float(i)).rgb * weight[i];
    }
    FragColor = vec4(color, 1.0);
}
