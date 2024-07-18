{ system ? builtins.currentSystem
, rust-overlay ? import (builtins.fetchTarball {
    url = https://github.com/oxalica/rust-overlay/archive/095702e63a40e86f339d11864da9dc965b70a01e.tar.gz;
    sha256 = "0q8bhriprl2g8im4civ4nvhamyh6y0j41z8q173psb4l6b5gwc9k";
  })
, cargo2nix ? (import (builtins.fetchTarball {
    url = https://github.com/cargo2nix/cargo2nix/archive/release-0.12.tar.gz;
    sha256 = "0dh2vwh1bj35jg001ykml1mw1c6ij7z0pwx3rf6ywxvl9xngpsp6";
  })).overlay
, pkgs ? (import <nixpkgs> {
    inherit system;
    overlays = [ cargo2nix rust-overlay ];
  })
}:
let
  rustPkgs = pkgs.rustBuilder.makePackageSet {
    rustVersion = "1.78.0";
    packageFun = import ./Cargo.nix;
#    localPatterns = [
#      ''^(src|tests|templates||include|)(/.*)?''
#      ''[^/]*\.(rs|toml|sql|h)$''
#    ];
    packageOverrides = pkgs: pkgs.rustBuilder.overrides.all ++ [
      (pkgs.rustBuilder.rustLib.makeOverride {
        name = "nitro-cli";
        overrideAttrs = drv: {
          LIBCLANG_PATH = "${pkgs.llvmPackages.libclang.lib}/lib";
          BINDGEN_EXTRA_CLANG_ARGS = ''
            -I${pkgs.glibc.dev}/include
            -idirafter ${pkgs.stdenv.cc.cc}/lib/clang/${pkgs.lib.getVersion pkgs.stdenv.cc.cc}/include
            -isystem ${pkgs.llvmPackages.libclang.lib}/lib/clang/${pkgs.lib.getVersion pkgs.clang}/include
          '';

          nativeBuildInputs = drv.nativeBuildInputs or [ ] ++ [ ];
          propagatedBuildInputs = drv.propagatedBuildInputs or [ ] ++ [ ];
        };
      })
    ];
  };
  nitro-cli-bin = (rustPkgs.workspace.nitro-cli { }).bin;
  vsock-proxy = (rustPkgs.workspace.vsock-proxy { }).bin;
  nitro-cli = pkgs.stdenv.mkDerivation {
    name = "nitro-cli";
    phases = [ "installPhase" ];
    src = pkgs.lib.sourceByRegex  ./. [''^(blobs|vsock_proxy)(/.*)?''];
    installPhase = ''
      mkdir -p $out/bin
      mkdir -p $out/blobs
      mkdir -p $out/enclave
      mkdir -p $out/etc/nitro_enclaves
      mkdir -p $out/run/nitro_enclaves
      ls $src $src/blobs
      cp $src/blobs/x86_64/* $out/blobs
      cp $src/vsock_proxy/configs/* $out/etc/nitro_enclaves/
      cp ${nitro-cli-bin}/bin/nitro-cli $out/bin/nitro-cli
      cp ${vsock-proxy}/bin/vsock-proxy $out/bin/vsock-proxy
    '';
    outputs = [ "out" ];
  };
in
{
  inherit nitro-cli;
}
