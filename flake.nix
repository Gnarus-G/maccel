{
  description = "maccel NixOS Module";

  outputs = {...}: {
    nixosModules.default = import ./module.nix;
  };
}
