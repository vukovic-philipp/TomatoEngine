{
  description = "SDL3 C++ Game Engine";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      devShells.${system}.default = pkgs.mkShell {
        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          pkg-config
          glslang
          clang-tools   # clangd, clang-format
          gdb
        ];
        buildInputs = with pkgs; [
          sdl3
          glm
        ];

        CMAKE_PREFIX_PATH = pkgs.lib.concatStringsSep ":" [
                  "${pkgs.sdl3.dev}"
                  "${pkgs.glm}"
                ];
      };
    };
}
