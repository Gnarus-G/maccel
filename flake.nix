{
  description = "maccel mouse acceleration driver and CLI";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = {
    self,
    nixpkgs,
    ...
  }: let
    system = "x86_64-linux";
    pkgs = import nixpkgs {inherit system;};
    moduleEvaluation = nixpkgs.lib.nixosSystem {
      inherit system;
      modules = [
        self.nixosModules.default
        {
          hardware.maccel = {
            enable = true;
            enableCli = true;
          };
          system.stateVersion = "24.11";
        }
      ];
    };
  in {
    packages.${system} = {
      maccel-cli = pkgs.callPackage ./package.nix {};
      default = self.packages.${system}.maccel-cli;
    };
    checks.${system} = {
      default = self.packages.${system}.default;
      nixos-module = let
        installedPackages = map nixpkgs.lib.getName moduleEvaluation.config.environment.systemPackages;
      in
        assert builtins.elem "maccel-cli" installedPackages;
          pkgs.runCommand "maccel-module-check" {} ''
            touch $out
          '';
    };

    nixosModules.default = import ./module.nix;
  };
}
