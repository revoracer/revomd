# revomd

[![CI](https://github.com/revoracer/revomd/actions/workflows/ci.yml/badge.svg)](https://github.com/revoracer/revomd/actions/workflows/ci.yml)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](#quick-start)
[![Header-only](https://img.shields.io/badge/dependencies-none-lightgrey.svg)](#requirements)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)

Normalized market-data domain vocabulary and wire schema for low-latency
data-path infrastructure.

`revomd` is a small, header-only C++20 library that defines the types
RevoRacer's market-data producers and consumers agree on: fixed-point
price/quantity, book side and level-mutation semantics, trade/quote/order
payloads, and the on-disk journal/wire layout those payloads use. It has no
runtime dependencies and no platform-specific code.

## Why this repo exists

A normalized market-data schema is the kind of shared leaf that is easy to
get wrong by ownership rather than by content: if the feed/codec layer owns
it, a backtester reading journals has to link the live feed handler; if an
order-book library owns it, every other consumer (simulation, analytics,
strategy) ends up depending on that one book just to read a trade. `revomd`
is deliberately owned by neither — it depends on nothing, and everything else
depends on it.

RevoRacer publishes `revomd` so the vocabulary underneath our market-data
path — what a price is, what a valid book side is, how a trade is framed on
the wire, how a venue message gets a stable numeric tag — is inspectable and
reusable on its own, independent of any particular producer or consumer
implementation.

## What is inside

| Header | Contents | Depends on venues/transport? |
| --- | --- | --- |
| `include/revomd/types.h` | Domain vocabulary: `Price`, `Quantity`, `Side`, `Action`, `AggressorSide`, `MboAction`, `QuoteLevel`, `RestingOrder`, `LevelUpdate`, and the `is_valid()` validity predicates. | No |
| `include/revomd/wire.h` | Persisted journal/wire schema: `EventHeader`, `Trade`/`TradeData`, `Bbo`/`BboData`, `BookChunk`/`BookLevel`, `MboEvent`/`MboData`, feed identity (`Venue`, `SourceId`, `InstrumentDefinition`, `SourceDefinition`), `TickTable`, raw/native frame headers, and the `MsgType` frame-tag registry. | Yes (includes `types.h`) |

The split is the point: a consumer that only needs the vocabulary (an
order-book implementation, say) includes `types.h` and never acquires an
opinion about framing or venue identity. A producer, replay path, or decode
layer includes `wire.h` too.

## Wire format stability

Every struct in `wire.h` is asserted standard-layout and trivially copyable,
and several carry exact `sizeof`/`offsetof` assertions at the bottom of the
file. `MsgType` values and struct layouts are written verbatim into journal
frames — **changing a field changes the on-disk format of every existing
capture.** `kSchemaVersion` exists to be bumped when that happens.

`Side` has a deliberately reserved, unnamed `0`: it is written into wire and
journal records, so leaving zero unnamed means a zero-filled or truncated
record reads as invalid rather than silently decoding as a valid `Bid`.
`is_valid(Side)` / `is_valid(Action)` are the sanctioned check at a decode
boundary — where untrusted bytes become domain values — not on a hot apply
path that has already established validity.

## Status

This is an early public split of a schema module used internally by
RevoRacer's market-data producer and consumer libraries. The API is not yet
stable; prefer additive changes (new fields, new `MsgType` values) to
renumbering or reordering existing ones.

Design limits:

- `revomd` is a schema/vocabulary library, not a transport, broker, order
  book, or venue codec.
- No runtime dependencies. `revomd` does not depend on, or require, any other
  RevoRacer repository. (Code that journals `wire.h` records through a
  transport such as [`revoq`](https://github.com/revoracer/revoq) lives in a
  separate producer layer, not here.)
- No platform-specific code — anything with a conformant C++20 compiler
  should work, though CI currently validates Linux (GCC, Clang) only.

## Requirements

- CMake 3.23 or newer
- A C++20-conformant compiler
- No runtime dependencies. The Catch2 submodule is needed only to build tests.

## Quick Start

```bash
git submodule update --init --recursive   # only needed to build tests
./build.sh
./build.sh test
```

`build.sh test` configures, builds, and runs the C++ test suite via `ctest`.

## Build Commands

```bash
./build.sh                 # Release build
./build.sh debug           # Debug build
./build.sh test            # Build and run tests
./build.sh clean           # Remove build/
./build.sh doctor          # Print tool versions
```

Use `--build-dir DIR` to select a different build directory, `-j N` or
`--jobs N` to control parallelism, and `--no-tests` to skip building the test
suite (and its Catch2 dependency) entirely.

## CMake Usage

From a parent CMake project:

```cmake
add_subdirectory(external/revomd)
target_link_libraries(my_target PRIVATE revomd::revomd)
```

From an installed package:

```cmake
find_package(revomd CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE revomd::revomd)
```

Public headers are under `include/revomd` and should be included with the
module prefix:

```cpp
#include <revomd/types.h>
#include <revomd/wire.h>
```

## Example

```cpp
#include <revomd/types.h>
#include <revomd/wire.h>

#include <cstdint>
#include <iostream>

int main() {
  // Domain vocabulary: no transport or venue concepts involved.
  revomd::LevelUpdate level{
      .price = 100'000, .quantity = 25, .side = revomd::Side::Bid};

  if (!revomd::is_valid(level.side)) {
    std::cerr << "corrupt or unparsed level\n";
    return 1;
  }

  // Wire schema: the same five fields, framed for the journal.
  revomd::BookDeltaBatch32 chunk{};
  chunk.levels[0] = level;
  chunk.num_levels = 1;
  chunk.num_bids = 1;

  std::cout << "schema version " << chunk.h.schema_version
            << ", frame tag " << static_cast<int>(revomd::MsgType::DiffDepth)
            << ", level price " << chunk.levels[0].price << "\n";
}
```

## Development

The test target enables `-Wall -Wextra -Wpedantic` on GCC, Clang, and Apple
Clang. `types.h` and `wire.h` themselves take no compiler flags beyond the
C++20 language standard, since `revomd` ships no compiled translation units.

## Commercial

RevoRacer builds low-latency trading infrastructure for hedge funds and
trading teams. Open-source `revomd` is intended to make the shared vocabulary
underneath that infrastructure visible: what the schema is, how it is
versioned, and how producer and consumer layers are kept decoupled from it.

Contact us through [contact@revoracer.com](mailto:contact@revoracer.com).

## License

`revomd` is licensed under Apache-2.0.
