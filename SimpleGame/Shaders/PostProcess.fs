#version 330 core
in vec2 v_UV;
uniform sampler2D u_Scene;
uniform sampler2D u_Blurred;
uniform bool u_ToneMapping;
uniform float u_Exposure;
uniform float u_Vignette;
uniform float u_BlurStrength;
uniform vec2 u_BlurRange;
layout(location = 0) out vec4 FragColor;

vec3 Filmic(vec3 x)
{
    // ACES-style fitted curve, not the full ACES color-management pipeline.
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

vec3 LinearToSRGB(vec3 color)
{
    return mix(
        color * 12.92,
        1.055 * pow(max(color, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055,
        step(vec3(0.0031308), color));
}

void main()
{
    // UV-relative elliptical mask: stable framing across resolution changes.
    float radius = length((v_UV - 0.5) * 2.0) * 0.70710678;
    float edge = smoothstep(u_BlurRange.x, u_BlurRange.y, radius);
    vec3 color = mix(texture(u_Scene, v_UV).rgb, texture(u_Blurred, v_UV).rgb, edge * u_BlurStrength);
    color *= u_Exposure;
    float vignette = smoothstep(0.30, 1.0, radius);
    color *= 1.0 - u_Vignette * vignette * vignette;
    if (u_ToneMapping)
    {
        color = Filmic(color);
    }
    // SDR display output. Linear HDR data stays unclamped until this final stage.
    FragColor = vec4(LinearToSRGB(clamp(color, 0.0, 1.0)), 1.0);
}
