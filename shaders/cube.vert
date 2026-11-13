#version 450
layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec3 v_world_pos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_color;
layout(location = 3) out vec2 v_uv;

layout(set = 1, binding = 0) uniform UBO {
    mat4 mvp;
    mat4 model;
    mat4 normal_mat;   // mat4 on purpose: avoids std140 mat3 padding trouble
} ubo;

void main() {
    vec4 world = ubo.model * vec4(in_pos, 1.0);
    gl_Position = ubo.mvp * vec4(in_pos, 1.0);
    v_world_pos = world.xyz;
    v_normal    = mat3(ubo.normal_mat) * in_normal;
    v_color     = in_color;
    v_uv        = in_uv;
}