## Dependencies:
- `libcurl`
- `libgit2`
- `libjson-c`

## How To Install
```
make
sudo make install
```

## How To Use
Either run `man aureate` or `aureate --help`

## To Do
- ~~Handle multiple packages to install at at time~~
- ~~Add `-R` flag~~
- ~~Parse package info with `-Ss`~~
- Clean up parse code
- Reimplement `strlen()` inside of `char` combined with `snprintf()` all using `asprintf()`
- Replace `system()` command with `exec()` family of functions
- Fix `flags` function to use `getopt()`
- ~~Use libgit to pull from the AUR git repos instead of redownloading tarball every time~~