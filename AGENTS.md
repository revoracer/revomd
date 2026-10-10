# AGENTS.md

Instructions for coding agents working in `revomd`.

## Project role

`revomd` is the normalized market-data schema leaf shared across RevoRacer's
data-path libraries. It defines the vocabulary and depends on nothing itself
(header-only, STL only). It is deliberately owned by neither a producer
(feed/codec layer) nor a consumer (an order-book implementation): if a
producer owned it, a backtester reading journals would have to link the live
feed handler; if a consumer owned it, every other consumer would depend on
that one to read a trade.

Do not add transport, broker, order-book, or venue-codec logic here. Those
belong in higher-level, layer-specific libraries that depend on `revomd`, not
the other way around.

## Library shape

Two headers, split on purpose:

- `include/revomd/types.h` — domain vocabulary. `Price`, `Quantity`, `Side`,
  `Action`, `AggressorSide`, `MboAction`, `QuoteLevel`, `RestingOrder`,
  `LevelUpdate`, and the `is_valid()` predicates. No transport or venue
  concepts. A pure consumer includes only this header and never acquires an
  opinion about framing or venues.
- `include/revomd/wire.h` — the journal/wire schema: how those domain values
  are framed, sequenced, and identified on disk. `EventHeader`, `Trade`/
  `TradeData`, `Bbo`/`BboData`, `BookChunk`/`BookLevel`, `MboEvent`/
  `MboData`, feed identity (`Venue`, `SourceId`, `InstrumentDefinition`,
  `SourceDefinition`), `TickTable`, raw/native frame headers, and the
  `MsgType` frame-tag registry. Includes `types.h`.

## Wire format is persisted — read this before touching `wire.h`

Every struct in `wire.h` is asserted standard-layout and trivially copyable,
several carry exact `sizeof`/`offsetof` assertions, and `MsgType` values are
written verbatim into journal frames. **Changing a field, reordering a
struct, or renumbering a `MsgType` changes the on-disk format of every
existing capture.**

- Prefer adding a new field or a new `MsgType` value over changing an
  existing one.
- Any layout change must bump `kSchemaVersion` and update or add the
  corresponding `static_assert`s at the bottom of `wire.h`.
- `Side` has a deliberate reserved-invalid `0` (see the comment above `enum
  class Side` in `types.h`) so a zero-filled or truncated record reads as
  invalid rather than silently decoding as a valid `Bid`. Do not add a third
  `Side` enumerator or reuse `0`.
- `is_valid(Side)` / `is_valid(Action)` are the sanctioned check at a decode
  boundary (untrusted bytes becoming domain values). They are not meant to be
  called on the hot apply path, where the caller has already established
  validity.

## Build and test

`revomd` itself has no runtime dependencies. The test suite needs the bundled Catch2 submodule:

```bash
git submodule update --init --recursive
./build.sh doctor
./build.sh test
```

`./build.sh test` configures, builds, and runs the C++ test suite through
`ctest`.

## Platform assumptions

- Language level: C++20.
- No platform-specific code. Any conformant C++20 compiler should work; CI
  validates GCC and Clang on Linux.

## Public API boundaries

Public headers live under `include/revomd/` and are included as:

```cpp
#include <revomd/types.h>
#include <revomd/wire.h>
```

Preserve the `revomd` namespace and the `types.h` / `wire.h` split unless an
API change is deliberate and documented. A consumer-facing library (an order
book, for example) should only ever need to include `types.h`.

## Editing guidance

- Keep `types.h` free of anything that only makes sense once you know how a
  value is transported (sequence numbers, source IDs, schema version, venue
  identity). Those belong in `wire.h`.
- Add or update tests when behavior changes.
- Do not change enum values, struct layout, or field order in `wire.h`
  without reading the `static_assert`s at the bottom of the file first — they
  exist specifically to catch an accidental layout regression.
- Keep README claims tied to code or tests in this repo.

## Files worth reading

- `README.md` — project overview, build instructions, CMake usage.
- `include/revomd/types.h` — domain vocabulary and the `Side`/`Action`
  validity contract.
- `include/revomd/wire.h` — persisted wire/journal schema, the `MsgType`
  registry, and the layout `static_assert`s.
- `tests/` — expected behavior and layout-stability checks.
