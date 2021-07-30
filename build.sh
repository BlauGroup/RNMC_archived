$CC ./src/*.c -o RNMC $(gsl-config --cflags) $(gsl-config --libs) -lsqlite3 -lpthread
