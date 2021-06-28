with (import <nixpkgs> {});

mkShell rec {
  buildInputs = [ clang
                  gcc
                  gsl
                  lldb
                  gdb
                  valgrind
                  meson
                  ninja
                  sqlite
                  sqlitebrowser
                ];

  # currently, clangd is not well wrapped in nixos.
  # CC=clang meson setup --buildtype=debug build
  # symbolic link compile_commands.json into root dir
  # this lets clangd find all project headers
  # setting CPATH explicitly as follows lets clangd find system headers

  CPATH = builtins.concatStringsSep ":" [
    "${glibc.dev}/include"
    "${llvmPackages.clang-unwrapped.lib}/lib/clang/7.1.0/include"
    "${sqlite.dev}/include"
    "${gsl}/include"
  ];

}
