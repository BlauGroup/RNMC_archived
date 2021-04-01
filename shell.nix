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
                ];

  CPATH = lib.makeSearchPathOutput "dev" "include" [ clang.libc gsl sqlite ];
}
