{ system ? builtins.currentSystem
, rust-overlay ? import (builtins.fetchTarball {
    url = https://github.com/oxalica/rust-overlay/archive/1b723f746e48ea9500007d17c41ad19441b88276.tar.gz;
    sha256 = "1gx1528zp3sj9dk53szmi1pab8jjqmlmrbxygbx9ak33bq9nsiv1";
  })
, cargo2nix ? import "${(builtins.fetchTarball {
    url = https://github.com/sphw/cargo2nix/archive/8edd83a16b4dc304c23bcee48f77894863c1eab4.tar.gz;
    sha256 = "08jai58alwyi55v918dp6n9fzdnv2iqb0ixs91l7r4hs38bz2aca";
  })}/overlay"
, pkgs ? (import <nixpkgs> {
    inherit system;
    overlays = [ cargo2nix rust-overlay ];
  })
}:
let
  rustPkgs = pkgs.rustBuilder.makePackageSet' {
    rustChannel = "1.54.0";
    packageFun = import ./Cargo.nix;
    localPatterns = [
      ''^(src|tests|templates||include|)(/.*)?''
      ''[^/]*\.(rs|toml|sql|h)$''
    ];
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

