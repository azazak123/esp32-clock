{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    utils.url = "github:numtide/flake-utils";
    esp-dev.url = "github:mirrexagon/nixpkgs-esp-dev";
  };

  outputs = { self, nixpkgs, utils, esp-dev }:
    utils.lib.eachSystem [ "x86_64-linux" ] (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShell = with pkgs; mkShell {
          buildInputs = [
            esp-dev.packages.${system}.esp-idf-full
            pkgs.nixpkgs-fmt
            pkgs.udev
          ];

          shellHook = ''
            export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${pkgs.lib.makeLibraryPath [
              pkgs.udev
            ]}"
          '';
        };
      });
}
