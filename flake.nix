{
  inputs.nixpkgs-esp-dev.url = "github:mirrexagon/nixpkgs-esp-dev";

  outputs = { self, nixpkgs-esp-dev }: {
    devShells.x86_64-linux.default =
      nixpkgs-esp-dev.devShells.x86_64-linux.esp-idf-full;
  };
}