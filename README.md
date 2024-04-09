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
- [x] Handle multiple packages to install at at time
- [x] Add `-R` flag
- [x] Parse package info with `-Ss`
- [ ] Clean up parse code
- [x] Reimplement `strlen()` inside of `char` combined with `snprintf()` all using `asprintf()`
- [x] Replace `system()` command with `exec()` family of functions
- [x] Fix `flags` function to use `getopt()`
- [x] Use libgit to pull from the AUR git repos instead of redownloading tarball every time
- [x] Properly wrap lines of `search()` output
- [ ] `-Syu` function to update all packages
  - Compare clone to master with `.git/refs/heads/master`
- [x] Make formatting consistent across all code
- [x] Add `-e` flag to edit PKGBUILD before install