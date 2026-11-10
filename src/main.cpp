#include "renderer/renderer.hpp"

int main(int argc, char* argv[]) {
    if (argc > 1) {return tomato::renderer::renderer(argv[1]);}
    return tomato::renderer::renderer("assets/BoxTextured.glb");
}
