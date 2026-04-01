{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }: let
    forAllSystems = function:
      nixpkgs.lib.genAttrs [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
      ] (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};

          isDarwinAarch64 =
            pkgs.stdenv.hostPlatform.isDarwin &&
            pkgs.stdenv.hostPlatform.isAarch64;
        in
        function pkgs isDarwinAarch64
      );
  in {
    devShells = forAllSystems (pkgs: isDarwinAarch64: {
      default = pkgs.mkShell {
        hardeningDisable = [ "fortify" ];
        packages = with pkgs;
          [
            compiledb
            gcovr
          ] ++ (if !isDarwinAarch64 then [
            sfml
            ncurses
            pkg-config
            SDL2
            SDL2_image
          ] else [
            libcxx
          ]) ++ [
            criterion
            clang
            clang-tools
          ];
      };
    });

    formatter = forAllSystems (pkgs: _: pkgs.alejandra);

    packages = forAllSystems (pkgs: _: {
      default = self.packages.${pkgs.system}._scom;
      _scom = pkgs.callPackage ./nix/package.nix { };
    });
  };
}
