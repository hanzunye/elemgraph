{
  description = "elemgraph: elementary graph algorithms in C++20";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});

      # Configures, builds and runs the test suite with the compiler of `stdenv`.
      mkTests = pkgs: { name, stdenv, sanitize ? false }:
        stdenv.mkDerivation {
          pname = "elemgraph-tests-${name}";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
          buildInputs = [ pkgs.gtest ];

          cmakeBuildType = if sanitize then "Debug" else "Release";
          cmakeFlags = [ (pkgs.lib.cmakeBool "ELEMGRAPH_SANITIZE" sanitize) ];

          # nixpkgs adds hardening flags (e.g. _FORTIFY_SOURCE) to every compiler
          # call. Turn them off so that builds see exactly the flags from CMake.
          hardeningDisable = [ "all" ];

          doCheck = true;
          checkPhase = ''
            runHook preCheck
            ctest --output-on-failure
            runHook postCheck
          '';

          # A check only has to succeed; there is nothing to install yet.
          installPhase = "touch $out";
        };
    in
    {
      checks = forAllSystems (pkgs:
        let tests = mkTests pkgs; in
        if pkgs.stdenv.hostPlatform.isLinux then {
          gcc = tests { name = "gcc"; stdenv = pkgs.gccStdenv; };
          clang = tests { name = "clang"; stdenv = pkgs.clangStdenv; };
          sanitize = tests { name = "sanitize"; stdenv = pkgs.gccStdenv; sanitize = true; };
        } else {
          # On macOS, nixpkgs builds GoogleTest with clang and libc++,
          # which a GCC/libstdc++ build cannot link against.
          clang = tests { name = "clang"; stdenv = pkgs.stdenv; };
        });

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          packages = [ pkgs.cmake pkgs.ninja pkgs.gtest pkgs.clang-tools ];
          hardeningDisable = [ "all" ];
        };
      });
    };
}
