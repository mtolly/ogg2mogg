Builds correct MOGG files for Harmonix games using `libvorbisfile`.

Windows build instructions:

  1. Install MSYS2, update with `pacman -Syu` until done

  2. `pacman -Sy mingw-w64-x86_64-libvorbis mingw-w64-x86_64-gcc pkg-config make`

  3. For a static build, go to `C:\msys64\mingw64\lib`, and remove: `libogg.dll.a`, `libvorbis.dll.a`, `libvorbisfile.dll.a`

  4. `make`
