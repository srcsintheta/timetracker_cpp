g++ -std=c++20 -Wall -Wpedantic -Werror -Wconversion \
    *.cpp -o tracker -lfmt -lsoci_core -lsoci_sqlite3 -L/usr/local/lib
