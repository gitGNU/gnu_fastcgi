/* Build instructions for the continuous integration system Hydra. */

{ fastcgiSrc ? { outPath = ./.; revCount = 0; gitTag = "dirty"; }
, officialRelease ? false
}:

let
  pkgs = import <nixpkgs> { };
  version = fastcgiSrc.gitTag;
  versionSuffix = "";
in
rec {

  tarball = pkgs.releaseTools.sourceTarball {
    name = "fastcgi-tarball";
    src = fastcgiSrc;
    inherit version versionSuffix officialRelease;
  };

  build = pkgs.lib.genAttrs [ "x86_64-linux" ] (system:
    let pkgs = import <nixpkgs> { inherit system; }; in
    pkgs.releaseTools.nixBuild {
      name = "fastcgi";
      src = tarball;
      buildInputs = [ pkgs.boostHeaders ];
    });
}
