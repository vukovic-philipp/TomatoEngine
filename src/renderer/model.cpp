#include "model.hpp"

#define CGLTF_IMPLEMENTATION
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "third_party/cgltf.h"

namespace tomato::model {

    static const cgltf_accessor* find_attr(const cgltf_primitive* p, cgltf_attribute_type type) {
        for (size_t i = 0; i < p->attributes_count; ++i) {
            if (p->attributes[i].type == type && p->attributes[i].index == 0) {
                return p->attributes[i].data;
            }
        }
        return nullptr;
    }

    static bool primitve_usable(const cgltf_primitive* p) {
        return p->type == cgltf_primitive_type_triangles && find_attr(p, cgltf_attribute_type_position) != nullptr;
    }

    static size_t primitive_index_count(const cgltf_primitive* p) {
        return p->indices ? p->indices->count : find_attr(p, cgltf_attribute_type_position)->count;
    }

    bool model_load(MeshData* out, const char* path) {
        *out = {};

        cgltf_options opts {};
        cgltf_data* data = nullptr;

        if (cgltf_parse_file(&opts, path, &data) != cgltf_result_success) {
            SDL_Log("CGLTF: Could not parse %s", path);
            return false;
        }
        if (cgltf_load_buffers(&opts, data, path) != cgltf_result_success) {
            SDL_Log("CGLTF: Could not load buffers for %s", path);
        }

        // count and alloc memory (pass 1)
        size_t vtotal = 0, itotal = 0;
        for (size_t n = 0; n < data->nodes_count; ++n) {
            const cgltf_mesh* mesh = data->nodes[n].mesh;
            if (!mesh) continue;

            for (size_t p = 0; p < mesh->primitives_count; ++p) {
                const cgltf_primitive* prim = &mesh->primitives[p];
                if (!primitve_usable(prim)) continue;
                vtotal += find_attr(prim, cgltf_attribute_type_position)->count;
                itotal += primitive_index_count(prim);
            }
        }

        if (vtotal == 0 || itotal == 0) {
            SDL_Log("CGLTF: %s has no triangle geometry", path);
            cgltf_free(data);
            return false;
        }

        out->verts = static_cast<pipelines::Vertex*>(SDL_malloc(vtotal * sizeof(pipelines::Vertex)));
        out->idx = static_cast<uint32_t*>(SDL_malloc(itotal * sizeof(uint32_t)));
        out->vcount = 0;
        out->icount = 0;

        // fill data (pass 2)
        for (size_t n = 0; n < data->nodes_count; ++n) {
            const cgltf_node* node = &data->nodes[n];
            if (!node->mesh) continue;

            float m[16];
            cgltf_node_transform_world(node, m);
            const glm::mat4 world = glm::make_mat4(m);
            // Mirrored transform flip winding, fix per triangle
            const bool flip = glm::determinant(glm::mat3(world)) < 0.0f;

            for (size_t p = 0; p < node->mesh->primitives_count; ++p) {
                const cgltf_primitive* prim = &node->mesh->primitives[p];
                if (!primitve_usable(prim)) continue;

                const cgltf_accessor* pos = find_attr(prim, cgltf_attribute_type_position);
                const cgltf_accessor* col = find_attr(prim, cgltf_attribute_type_color);
                const cgltf_accessor* uvs = find_attr(prim, cgltf_attribute_type_texcoord);

                glm::vec3 base(1.0f);
                if (prim->material && prim->material->has_pbr_metallic_roughness) {
                    base = glm::make_vec3(prim->material->pbr_metallic_roughness.base_color_factor);
                }

                const uint32_t base_vertex = out->vcount;

                for (size_t v = 0; v < pos->count; ++v) {
                    float pv[3] {};
                    cgltf_accessor_read_float(pos, v, pv, 3);

                    glm::vec3 color = base;
                    if (col) {
                        float c[4] = {1, 1, 1, 1};
                        cgltf_accessor_read_float(col, v, c, 4);
                        color = glm::vec3(c[0], c[1], c[3]);
                    }

                    glm::vec2 uv(0.0f);
                    if (uvs) {
                        float t[2] = {};
                        cgltf_accessor_read_float(uvs, v, t, 2);
                        uv = glm::vec2(t[0], t[1]);
                    }

                    pipelines::Vertex& dst = out->verts[out->vcount++];
                    dst.pos = glm::vec3(world * glm::vec4(pv[0], pv[1], pv[2], 1.0f));
                    dst.color = color;
                    dst.uv = uv;
                }

                const size_t icount = primitive_index_count(prim);
                for (size_t i = 0; i < icount; ++i) {
                    size_t src = prim->indices ? cgltf_accessor_read_index(prim->indices, i) : i;
                    out->idx[out->icount++] = base_vertex + static_cast<uint32_t>(src);
                }
                if (flip) {
                    for (size_t i = 0; i + 2 < icount; i += 3) {
                        uint32_t* tri = &out->idx[out->icount - icount + 1];
                        uint32_t tmp = tri[1];
                        tri[1] = tri[2];
                        tri[2] = tmp;
                    }
                }
            }
        }

        cgltf_free(data);
        return true;
    }

    void model_free(MeshData* mesh) {
        SDL_free(mesh->verts);
        SDL_free(mesh->idx);
        *mesh = {};
    }

}
