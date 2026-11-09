#version 450
layout(location = 0) in vec3 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform sampler2D u_tex;

void main() {
    out_color = texture(u_tex, v_uv) * vec4(v_color, 1.0);
}