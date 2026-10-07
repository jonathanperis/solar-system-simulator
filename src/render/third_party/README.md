# Vendored third-party code

## stb_image.h

- **Project:** stb by Sean Barrett, <https://github.com/nothings/stb>
- **Version:** stb_image v2.30, from commit `2c980bb59875b0d32144a71867fbdebb2f77cd20`
- **File SHA-256:** `594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3`
- **License:** dual MIT / public domain (Unlicense), as stated at the end of the file.
- **Why:** raylib is built with JPEG decoding disabled (`SUPPORT_FILEFORMAT_JPG 0`),
  and PNG copies of the planet maps would be roughly ten times larger.
- **How it is used:** only `src/render/image_decode.c` includes it, with
  `STB_IMAGE_STATIC` (internal linkage, so it cannot clash with raylib's own
  copy) and only the JPEG and PNG decoders compiled. It decodes the project's
  own pinned textures, never user-supplied files.

Update by replacing the file with a newer upstream release, recording the new
commit and SHA-256 here, and running `make test` (including `test_image_decode`)
and the browser suite.
