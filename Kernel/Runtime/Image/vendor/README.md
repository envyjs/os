# Image decoder sources

- [stb_image v2.30](https://github.com/nothings/stb/blob/2c980bb59875b0d32144a71867fbdebb2f77cd20/stb_image.h)
  (MIT / public domain, license included in the header).
- [NanoSVG parser](https://github.com/memononen/nanosvg/blob/239e102ec2c691f2902e20ace2ed36ee4a35cfe6/src/nanosvg.h)
  and [rasterizer](https://github.com/memononen/nanosvg/blob/239e102ec2c691f2902e20ace2ed36ee4a35cfe6/src/nanosvgrast.h)
  (zlib license, included in both headers).

Downloaded 2026-10-09. Local changes: `NSVG_NO_STDIO` guards the file-based
NanoSVG parser, and `STBI_JPEG_MAX_OUTPUT_DIMENSION` optionally downsamples
JPEG color-converted rows into a smaller output buffer. JPEG component and
progressive coefficient decoding retain the upstream implementation.

`../compat/` supplies the limited C-library and x87 math operations used by
these decoders. `../support.c` supplies a private resettable image heap with
reusable blocks. JPEG/PNG SIMD, HDR, linear conversion, host file I/O, and
thread-local storage are disabled. SVG compilation alone enables x87;
stage two initializes the FPU before entering C code.
