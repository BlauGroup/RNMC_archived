with (import <nixpkgs> {});

mkShell rec {
  buildInputs = [ gcc
                  gsl
                  sqlite
                  # lldb
                  # gdb
                  # valgrind
                  # clang
                  # sqlitebrowser
                ];

  # currently, clangd is not well wrapped in nixos.
  # setting CPATH explicitly as follows lets clangd find system headers

  # CPATH = builtins.concatStringsSep ":" [
  #   "${glibc.dev}/include"
  #   "${llvmPackages.clang-unwrapped.lib}/lib/clang/7.1.0/include"
  #   "${sqlite.dev}/include"
  #   "${gsl}/include"
  # ];

}
