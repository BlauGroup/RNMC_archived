with (import <nixpkgs> {});

mkShell rec {
  buildInputs = [ gcc
                  gsl
                  sqlite
                  # lldb
                  gdb
                  valgrind
                  clang
                ];

  CLANGD_PATH = builtins.concatStringsSep ":" [
    "${glibc.dev}/include"
    "${llvmPackages.clang-unwrapped.lib}/lib/clang/7.1.0/include"
    "${sqlite.dev}/include"
    "${gsl}/include"
  ];

}
