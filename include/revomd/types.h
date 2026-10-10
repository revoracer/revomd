#pragma once

// revomd — normalized market-data domain vocabulary.
//
// The schema leaf: this module DEFINES the firm-wide normalized types and
// depends on nothing. Everything else depends on it —
//
//   revofeeds  produces these from venue wire formats (codecs, normalizers)
//   revobook   consumes them (the book is one consumer among several)
//   sim / strategy / analytics consume them too
//
// The schema is deliberately owned by neither a producer nor a consumer. If
// the feed layer owned it, a backtester reading journals would have to link
// the live feed handler; if the book owned it, every consumer would depend on
// the book to read a trade. See notes/revobook_types_unification.md.
//
// This header is the DOMAIN vocabulary — value types with no transport or
// venue concepts in them. Journal/wire records (EventHeader, BookChunk, Trade,
// MboEvent, ...) and feed identity (Venue, SourceId, InstrumentDefinition)
// belong in the wire/feed layer, NOT here: revobook includes this header and
// must never acquire an opinion about framing or venues through it.
//
// No dependencies beyond <cstdint>, header-only.

#include <cstdint>

namespace revomd {

// Fixed-point integer price/quantity on the instrument's tick/lot grid. No
// float on the book path — the caller normalizes to integer ticks before
// apply(). Signedness is deliberate: prices may be negative (e.g. spreads,
// calendars) and int64 keys sit directly on the tick grid.
using Price = std::int64_t;
using Quantity = std::int64_t;

// Book side. 0 is deliberately NOT a valid side: these values are written
// verbatim into wire/journal records, and leaving zero unnamed means a
// zero-filled or truncated record reads as an invalid side rather than
// silently decoding as a valid Bid. Never used as an array index or cast to an
// integer — consumers branch on it symbolically — so the hole costs nothing.
//
// There is no Unknown enumerator: a two-valued Side is what lets the book's
// apply path branch bid/ask with no third case. Value-initialization
// (`Side s{}`) yields the reserved 0 when a "not yet parsed" state is needed,
// and is_valid() below is how a boundary checks it.
enum class Side : std::uint8_t { Bid = 1, Ask = 2 };

// Mutation kind carried by a LevelUpdate.
//   Upsert — insert or overwrite the level at `price` (quantity 0 ⇒ Delete).
//   Delete — remove the level at `price` (non-existent ⇒ silent no-op).
//   Clear  — drop the entire side (venue "clear book" / reseed boundary).
enum class Action : std::uint8_t { Upsert = 0, Delete = 1, Clear = 2 };

// Validity predicates for the enums that cross a wire/journal boundary.
//
// Call these where untrusted bytes become domain values — a codec decoding a
// venue message, or a replay reading a journal frame — NOT on the hot apply
// path, where the caller has already established validity. Without them the
// reserved-0 contract above is unenforceable: `Side` has no Unknown to compare
// against by design, so this is the sanctioned check.
[[nodiscard]] constexpr bool is_valid(Side s) noexcept {
  return s == Side::Bid || s == Side::Ask;
}

[[nodiscard]] constexpr bool is_valid(Action a) noexcept {
  return a == Action::Upsert || a == Action::Delete || a == Action::Clear;
}

// Which side initiated a trade. NOT a book side — a trade is a sibling of the
// book, not an input to it (an L2 diff already reflects the post-trade level;
// an L3 execution arrives as an order event, not a trade record). Unknown is a
// genuine state here: venues do not always report the aggressor.
enum class AggressorSide : std::uint8_t { Unknown = 0, Buy = 1, Sell = 2 };

// Order-granular (L3 / market-by-order) event kind, as carried by a normalized
// MBO record. Unknown = 0 mirrors the wire-safety convention above.
enum class MboAction : std::uint8_t {
  Unknown = 0,
  Add = 1,
  Modify = 2,
  Delete = 3,
  Execute = 4,
  Clear = 5,
};

// A resting level as read back out of a book (best / top(k) / iteration).
struct QuoteLevel {
  Price price{};
  Quantity quantity{};
  std::uint32_t order_count{};

  friend constexpr bool operator==(const QuoteLevel &,
                                   const QuoteLevel &) = default;
};

// A single resting order as seen by an L3 listener callback (on_order_add /
// modify / delete — see revobook/listener.h). POD, passed by const ref.
// This is the order-granular analogue of QuoteLevel: QuoteLevel is the derived
// price-level view, RestingOrder is the underlying order that caused the
// change.
struct RestingOrder {
  std::uint64_t id{};
  Side side{Side::Bid};
  Price price{};
  Quantity quantity{};

  friend constexpr bool operator==(const RestingOrder &,
                                   const RestingOrder &) = default;
};

// A single level mutation applied to a book. This is the input vocabulary of
// the MBP apply path; a feed codec fills it from a venue diff/snapshot level.
//
// This is ALSO the on-wire level record (revomd/wire.h aliases BookLevel to
// it), which is why `side` value-initializes to the reserved-invalid 0 rather
// than defaulting to Bid: a default-constructed LevelUpdate is not a
// meaningful update, and trailing unused slots in a chunk's level array should
// not read as a valid Bid. (RestingOrder keeps a Bid default — it is a
// read-back view, never a wire record.) Note this only governs
// value-initialization; bytes read off a journal carry whatever the writer
// wrote, so is_valid() is still the check at a decode boundary.
struct LevelUpdate {
  Price price{};
  Quantity quantity{};
  std::uint32_t order_count{};
  Side side{};
  Action action{Action::Upsert};
};

} // namespace revomd
