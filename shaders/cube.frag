#version 450
layout(location = 0) in vec3 v_world_pos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_color;
layout(location = 3) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform sampler2D u_tex;
layout(set = 3, binding = 0) uniform Light {   // SDL3 GPU: fragment uniforms are set 3
    vec4 dir_to_light;   // xyz = direction FROM surface TOWARD light
    vec4 color;
    vec4 ambient;
    vec4 cam_pos;
} light;

void main() {
    vec3 N = normalize(v_normal);          // re-normalize: interpolation shortens it
    vec3 L = normalize(light.dir_to_light.xyz);
    vec3 V = normalize(light.cam_pos.xyz - v_world_pos);
    vec3 H = normalize(L + V);

    float ndl  = max(dot(N, L), 0.0);
    float spec = ndl > 0.0 ? pow(max(dot(N, H), 0.0), 32.0) : 0.0;

    vec3 albedo = texture(u_tex, v_uv).rgb * v_color;
    vec3 lit = albedo * (light.ambient.rgb + light.color.rgb * ndl)
    + light.color.rgb * spec * 0.3;
    out_color = vec4(lit, 1.0);
}