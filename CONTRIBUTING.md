# Contributing

Thanks for improving `revomd`.

## Development Setup

`revomd` itself has no runtime dependencies; the test suite needs the bundled Catch2 submodule:

```bash
git submodule update --init --recursive
./build.sh test
```

Use `./build.sh doctor` if local toolchain details need to be checked.

## Pull Requests

- Keep changes focused on one type or build concern.
- Add or update tests when behavior changes.
- Preserve the public namespace and include layout (`<revomd/types.h>`,
  `<revomd/wire.h>`) unless the API change is intentional.
- Treat `include/revomd/wire.h` as an on-disk format, not a regular header:
  every struct in it is layout-asserted, and some carry exact `sizeof`/
  `offsetof` assertions. A field change there changes the byte layout of every
  existing capture — bump `kSchemaVersion` and call it out explicitly in the
  pull request description.
- `include/revomd/types.h` is pure domain vocabulary with no transport or
  venue concepts. Keep framing, feed-identity, and wire-record additions in
  `wire.h` instead.
- Run `./build.sh test` and include relevant failures if tests cannot be run.

The API is still early and may change, but changes should remain explicit and reviewable.
