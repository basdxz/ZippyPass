{
  description = "CMake based LLVM Plugin Dev";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }: let
    supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
    forEachSupportedSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
      pkgs = import nixpkgs { inherit system; };
    });
    in {
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell {
          # Disables Fortification, as it does not work with -O0
          # https://www.gnu.org/software/libc/manual/html_node/Source-Fortification.html
          hardeningDisable = [ "fortify" ];
          packages = with pkgs; [
            bc
            jq
            cmake
            ninja
            ccache
            clang_19
            llvm_19
          ];
          shellHook = ''
            export CMAKE_GENERATOR="Ninja"
            export CMAKE_MAKE_PROGRAM="${pkgs.ninja}/bin/ninja"

            export CC="${pkgs.clang_19}/bin/clang"
            export CXX="${pkgs.clang_19}/bin/clang++"
          '';
        };
      });
    };
}
