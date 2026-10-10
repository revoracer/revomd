#pragma once

// revomd — journal/wire records.
//
// The transport half of the schema: the byte layouts actually written to a
// revoq journal, plus feed identity (Venue, SourceId, InstrumentDefinition)
// and the chunk framing. Split from revomd/types.h on purpose —
//
//   types.h  domain vocabulary (Price, Side, QuoteLevel, LevelUpdate...)
//   wire.h   how those are framed, sequenced and identified on the wire
//
// so a CONSUMER that only needs the vocabulary (revobook) includes types.h and
// never acquires an opinion about framing or venues. A producer (revofeeds) or
// a replay/decode path includes this header too.
//
// LAYOUT IS PERSISTED. Every struct here is asserted standard-layout,
// trivially copyable and frame-alignment-safe at the bottom of this file, and
// several carry exact sizeof() assertions. Changing a field changes the
// on-disk format of every existing capture — bump kSchemaVersion and mean it.


#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <revomd/types.h>

namespace revomd {

// Frame type registry for normalized market-data payloads.
//
// This is the MD half of the numeric space carried in a journal frame's
// msg_type field. It lives here, not in the journal library, because a
// transport should move opaque bytes: with the registry baked into revoq you
// could not journal an order/risk/log record without dragging market-data
// concepts along. revoq keeps only the values it interprets itself (the
// segment marker, plus the session markers already on disk) — see
// revoq::MsgType for the full numeric allocation across modules, which
// is an append-only ledger and the place to claim a block before filling it.
//
// MD owns 100-9999. The sub-blocks below are allocated 100 at a time; new
// families continue at 500 and up. Per-venue message variants take NO values
// from this registry — they ride NativeEvent with venue + native_template_id
// carried inside NativeEventHeader, so adding a venue costs nothing here.
//
// VALUES ARE PERSISTED. Every existing capture carries these numbers in
// FrameHeader.msg_type; they are stable by contract. Prefer broad payload
// layouts over venue-specific variants when adding to the registry — several
// msg_types sharing one payload struct collapse to a single handler arm on the
// read path (SnapshotDepth/DiffDepth are the same BookChunk<N>, and the
// BookTop* widths ride it too).
enum class MsgType : std::uint16_t {
  // Normalized market data: 100-199.
  Trade = 100,
  SnapshotDepth = 101,
  DiffDepth = 102,
  BookTop5 = 103,
  BookTop10 = 104,
  BookTop20 = 105,
  BBO = 106,
  BookReset = 108,

  FeedStatus = 110,
  FeedGap = 111,
  FeedRecovery = 112,
  Heartbeat = 113,

  MarkPrice = 150,
  FundingRate = 151,
  OpenInterest = 152,
  Liquidation = 153,

  // Normalized order-book-by-order market data: 200-299.
  MboAdd = 200,
  MboModify = 201,
  MboDelete = 202,
  MboExecute = 203,
  MboSnapshot = 204,

  // Market/reference metadata: 300-399.
  InstrumentDefinition = 300,
  SourceDefinition = 301,
  TradingStatus = 302,
  TickTable = 303,

  // Raw/native feed preservation: 400-499.
  RawPacket = 400,
  RawMessage = 401,
  NativeEvent = 402,
};


// Level mutation kind as carried by a depth delta record. DeltaAction is the
// wire-side spelling of the domain Action in types.h — same type, and the
// feed-facing name is kept because that is how the venue documents it.
// using DeltaAction = Action;


using InstrumentId = std::uint64_t;
using SourceId = std::uint32_t;

inline constexpr std::uint16_t kSchemaVersion = 1;

enum class Venue : std::uint16_t {
  Unknown = 0,
  Binance = 1,
  Okx = 2,
  Coinbase = 3,
  Bitfinex = 4,

  Nyse = 100,
  Cme = 101,
  Hkex = 102,
};

enum class AssetClass : std::uint8_t {
  Unknown = 0,
  CryptoSpot = 1,
  CryptoPerp = 2,
  CryptoFuture = 3,
  CryptoOption = 4,
  Equity = 10,
  Future = 11,
  Option = 12,
};

enum class SourceProtocol : std::uint8_t {
  Unknown = 0,
  WebSocketJson = 1,
  WebSocketSbe = 2,
  RestSnapshot = 3,
  UdpMulticast = 10,
  TcpBinary = 11,
  FileReplay = 250,
};

enum class BookResetReason : std::uint8_t {
  Unknown = 0,
  Startup = 1,
  Gap = 2,
  Recovery = 3,
  Stale = 4,
  Crossed = 5,
  Manual = 6,
  VenueReset = 7,
  SnapshotRefresh = 8,
};

enum class FeedStatusCode : std::uint8_t {
  Unknown = 0,
  Connecting = 1,
  Connected = 2,
  Synchronizing = 3,
  Synchronized = 4,
  Resyncing = 5,
  Stale = 6,
  Disconnected = 7,
  Failed = 8,
};

enum class TradingStatusCode : std::uint8_t {
  Unknown = 0,
  PreOpen = 1,
  Open = 2,
  Halted = 3,
  Closed = 4,
  Auction = 5,
  PostClose = 6,
  Suspended = 7,
};

enum EventFlags : std::uint16_t {
  EventFlagNone = 0x0000,
  EventFlagSnapshot = 0x0001,
  EventFlagDelta = 0x0002,
  EventFlagDerived = 0x0004,
  EventFlagReplay = 0x0008,
  EventFlagGap = 0x0010,
  EventFlagRecovered = 0x0020,
  EventFlagStale = 0x0040,
  EventFlagTruncated = 0x0080,
  EventFlagOutOfOrder = 0x0100,
  EventFlagImplied = 0x0200,
  EventFlagIndicative = 0x0400,
  EventFlagNativeExtension = 0x0800,
  EventFlagMoreChunks = 0x1000,
  EventFlagLastChunk = 0x2000,
};

enum TradeFlags : std::uint16_t {
  TradeFlagNone = 0x0000,
  TradeFlagPrintable = 0x0001,
  TradeFlagBuyerMaker = 0x0002,
  TradeFlagLiquidation = 0x0004,
  TradeFlagAuction = 0x0008,
  TradeFlagBlock = 0x0010,
  TradeFlagCorrection = 0x0020,
  TradeFlagCancel = 0x0040,
};

template <std::size_t N> struct FixedText {
  std::array<char, N> bytes{};
  std::uint16_t size{};
  std::uint16_t _reserved{};
};

// Shared prefix for normalized market-data payloads.
struct EventHeader {
  InstrumentId instrument_id{};
  std::int64_t exchange_ts_ns{};    // venue event time, if present
  std::int64_t transaction_ts_ns{}; // venue match/transact time, if present
  std::int64_t receive_ts_ns{};     // best local RX timestamp available
  std::uint64_t first_sequence{};   // first venue sequence in this event
  std::uint64_t sequence{};         // last/current venue sequence
  std::uint64_t prev_sequence{};    // previous/link sequence, if present
  SourceId source_id{};             // feed/session/channel/source dictionary ID
  std::uint16_t flags{};
  std::uint16_t schema_version{kSchemaVersion};
};
static_assert(sizeof(EventHeader) == 64);

// The trade itself, with no feed/journal envelope. Split out so a consumer
// that wants a print — a book, a strategy, an analytics pass — can take the
// domain value without depending on SourceId, sequence numbers or
// schema_version, which are properties of how the event was TRANSPORTED, not
// of the trade.
//
// Field order and types are exactly as they were inline in Trade, so the
// on-disk layout is unchanged and existing captures decode field-for-field.
// The offsetof assertions below are what enforce that.
struct TradeData {
  std::uint64_t trade_id{};
  Price price{};       // fixed point per InstrumentDefinition
  Quantity quantity{}; // fixed point per InstrumentDefinition
  AggressorSide aggressor_side{AggressorSide::Unknown};
  std::uint8_t _pad0{};
  std::uint16_t trade_flags{};
  std::uint32_t native_condition{}; // venue-specific sale/match condition code
};

struct Trade {
  EventHeader h{};
  TradeData d{};
};

// Top-of-book, envelope-free. See TradeData for why the split exists; field
// order is unchanged so the on-disk layout is identical.
struct BboData {
  Price bid_price{};
  Quantity bid_quantity{};
  Price ask_price{};
  Quantity ask_quantity{};
  std::uint32_t bid_count{};
  std::uint32_t ask_count{};
};

struct Bbo {
  EventHeader h{};
  BboData d{};
};

// The wire level record IS the book's level-mutation type — same five fields,
// same byte layout — so a decode path hands journal bytes straight to a book
// with no conversion step. sizeof is unchanged at 24 because the trailing
// padding absorbs both, so existing captures still decode field-for-field.
using BookLevel = LevelUpdate;
static_assert(sizeof(BookLevel) == 24);

// Unified chunked book-event container. A snapshot chunk and a delta chunk
// share this exact layout and differ only by EventHeader::flags
// (EventFlagSnapshot vs EventFlagDelta) and stream MsgType (SnapshotDepth vs
// DiffDepth). Levels are interleaved — each BookLevel carries its own side —
// emitted bids-first (descending) then asks (ascending); snapshot levels use
// action == Upsert. Reassembly is one path for both: accumulate by
// level_offset/chunk_index and commit on EventFlagLastChunk. BookDeltaBatch /
// BookSnapshot below are width-specialized aliases of this one struct
template <std::size_t MaxLevels> struct BookChunk {
  EventHeader h{};
  std::uint16_t num_levels{}; // levels written in this chunk
  std::uint16_t num_bids{};   // bid levels in this chunk
  std::uint16_t num_asks{};   // ask levels in this chunk
  std::uint16_t _reserved0{};
  std::uint32_t total_levels{}; // original event total; set on final chunk
  std::uint32_t level_offset{}; // zero-based offset in original event
  std::uint32_t chunk_index{};
  // 0 until final chunk; final carries total chunks
  std::uint32_t chunk_count{};
  // pre-truncation per-side totals; set on final chunk
  std::uint32_t total_bids{};
  std::uint32_t total_asks{};
  std::array<BookLevel, MaxLevels> levels{};
};

template <std::size_t N> using BookDeltaBatch = BookChunk<N>;
template <std::size_t N> using BookSnapshot = BookChunk<N>;

struct BookReset {
  EventHeader h{};
  BookResetReason reason{BookResetReason::Unknown};
  std::uint8_t _pad0[7]{};
  std::uint64_t expected_snapshot_sequence{};
};

struct FeedStatus {
  EventHeader h{};
  FeedStatusCode status{FeedStatusCode::Unknown};
  std::uint8_t _pad0{};
  std::uint16_t native_status{};
  std::uint32_t detail_code{};
  std::uint64_t last_sequence{};
};

struct FeedGap {
  EventHeader h{};
  std::uint64_t expected_sequence{};
  std::uint64_t observed_sequence{};
  std::uint64_t missing_count{};
  std::uint32_t detail_code{};
  std::uint32_t _reserved0{};
};

struct FeedRecovery {
  EventHeader h{};
  std::uint64_t from_sequence{};
  std::uint64_t to_sequence{};
  FeedStatusCode status{FeedStatusCode::Unknown};
  std::uint8_t _pad0[7]{};
};

struct Heartbeat {
  EventHeader h{};
  std::uint64_t source_sequence{};
  std::int64_t expected_interval_ns{};
};

struct TradingStatus {
  EventHeader h{};
  TradingStatusCode status{TradingStatusCode::Unknown};
  std::uint8_t _pad0{};
  std::uint16_t native_status{};
  std::uint32_t reason_code{};
};

struct MarkPrice {
  EventHeader h{};
  Price mark_price{};
  Price index_price{};
  Price estimated_settle_price{};
  std::int64_t funding_rate{}; // fixed point; scale in InstrumentDefinition
  std::int64_t next_funding_ts_ns{};
};

struct FundingRate {
  EventHeader h{};
  std::int64_t funding_rate{}; // fixed point; scale in InstrumentDefinition
  std::int64_t funding_interval_ns{};
  std::int64_t next_funding_ts_ns{};
};

struct OpenInterest {
  EventHeader h{};
  Quantity open_interest{};
  Price notional_value{};
};

struct Liquidation {
  EventHeader h{};
  std::uint64_t order_id{};
  Price price{};
  Quantity quantity{};
  Side side{}; // 0 = reserved-invalid (was Side::Unknown)
  std::uint8_t _pad0[7]{};
};

// A single order-level (L3) event, envelope-free.
//
// NOTE: this is deliberately NOT revobook::RestingOrder, despite sharing
// {order_id, side, price, quantity}. RestingOrder declares those four
// contiguously; MboEvent's on-disk order interleaves `priority` between
// order_id and price and puts `side` after executed_quantity. The two stay
// separate types on purpose.
struct MboData {
  std::uint64_t order_id{};
  std::uint64_t priority{};
  Price price{};
  Quantity quantity{};
  Quantity executed_quantity{};
  Side side{}; // 0 = reserved-invalid (was Side::Unknown)
  MboAction action{MboAction::Unknown};
  std::uint16_t native_flags{};
  std::uint32_t _reserved0{};
};

struct MboEvent {
  EventHeader h{};
  MboData d{};
};

struct SourceDefinition {
  SourceId source_id{};
  Venue venue{Venue::Unknown};
  SourceProtocol protocol{SourceProtocol::Unknown};
  std::uint8_t line_id{};
  std::uint16_t channel_id{};
  std::uint32_t flags{};
  std::uint64_t session_id{};
  FixedText<32> name{};
  FixedText<64> endpoint{};
};

struct InstrumentDefinition {
  InstrumentId instrument_id{};
  SourceId source_id{};
  Venue venue{Venue::Unknown};
  AssetClass asset_class{AssetClass::Unknown};
  std::uint8_t price_scale{};
  std::uint8_t quantity_scale{};
  std::uint8_t rate_scale{};
  std::uint8_t _pad0{};
  std::uint64_t native_security_id{};
  Price min_price_increment{};
  Quantity quantity_increment{};
  std::int64_t contract_multiplier{};
  Quantity min_quantity{};
  Quantity max_quantity{};
  std::int64_t activation_ts_ns{};
  std::int64_t expiration_ts_ns{};
  std::uint32_t flags{};
  std::uint32_t _reserved0{};
  FixedText<32> symbol{};
  FixedText<16> base_asset{};
  FixedText<16> quote_asset{};
  FixedText<16> settlement_asset{};
};

struct TickTableEntry {
  Price low_price{};
  Price increment{};
};

template <std::size_t MaxEntries> struct TickTable {
  InstrumentId instrument_id{};
  SourceId source_id{};
  std::uint16_t num_entries{};
  std::uint16_t _reserved0{};
  std::array<TickTableEntry, MaxEntries> entries{};
};

struct RawMessageHeader {
  SourceId source_id{};
  SourceProtocol protocol{SourceProtocol::Unknown};
  std::uint8_t flags{};
  std::uint16_t schema_version{kSchemaVersion};
  std::int64_t receive_ts_ns{};
  std::uint64_t packet_sequence{};
  std::uint64_t message_sequence{};
  std::uint32_t payload_size{};
  std::uint32_t original_size{};
  std::uint32_t payload_hash{};
  std::uint32_t _reserved0{};
};

struct RawPacketHeader {
  SourceId source_id{};
  SourceProtocol protocol{SourceProtocol::Unknown};
  std::uint8_t flags{};
  std::uint16_t schema_version{kSchemaVersion};
  std::int64_t receive_ts_ns{};
  std::uint64_t packet_sequence{};
  std::uint32_t payload_size{};
  std::uint32_t original_size{};
  std::uint32_t payload_hash{};
  std::uint32_t _reserved0{};
};

struct NativeEventHeader {
  SourceId source_id{};
  Venue venue{Venue::Unknown};
  SourceProtocol protocol{SourceProtocol::Unknown};
  std::uint8_t flags{};
  std::uint16_t schema_version{kSchemaVersion};
  std::uint16_t native_template_id{};
  std::uint16_t native_schema_id{};
  std::uint16_t native_schema_version{};
  std::uint64_t packet_sequence{};
  std::uint64_t message_sequence{};
  std::uint32_t payload_size{};
  std::uint32_t payload_hash{};
};

using BookDeltaBatch32 =
    BookDeltaBatch<32>; // live delta width (plan: MaxLevels = 32)
using BookSnapshot256 =
    BookSnapshot<256>; // seed snapshot width (plan: 128-256, cold path)
using TickTable16 = TickTable<16>;

// alignof(T) <= 32: stream payloads land at frame_start + 32 (frame starts are
// 64-byte aligned), so a type demanding 64-byte alignment cannot be read in
// place.
#define REVOMD_POD(T)                                                    \
  static_assert(std::is_standard_layout_v<T>);                                 \
  static_assert(std::is_trivially_copyable_v<T>);                              \
  static_assert(alignof(T) <= 32,                                              \
                "journaled payload must satisfy frame+32 alignment")

REVOMD_POD(EventHeader);
REVOMD_POD(Trade);
REVOMD_POD(Bbo);
REVOMD_POD(BookLevel);
REVOMD_POD(BookReset);
REVOMD_POD(FeedStatus);
REVOMD_POD(FeedGap);
REVOMD_POD(FeedRecovery);
REVOMD_POD(Heartbeat);
REVOMD_POD(TradingStatus);
REVOMD_POD(MarkPrice);
REVOMD_POD(FundingRate);
REVOMD_POD(OpenInterest);
REVOMD_POD(Liquidation);
REVOMD_POD(MboEvent);
REVOMD_POD(SourceDefinition);
REVOMD_POD(InstrumentDefinition);
REVOMD_POD(TickTableEntry);
REVOMD_POD(RawMessageHeader);
REVOMD_POD(RawPacketHeader);
REVOMD_POD(NativeEventHeader);

// Chunked book event payloads written via openData<T>() — same alignment rule.
// Delta and snapshot are the same BookChunk<N> struct at different widths.
REVOMD_POD(BookDeltaBatch32);
REVOMD_POD(BookSnapshot256);

static_assert(sizeof(Trade) == 96);
// Layout guard for the envelope/payload split: the payload must still begin
// immediately after the header, and each field must sit where it sat when it
// was declared inline. A regression here silently misreads every capture.
static_assert(sizeof(TradeData) == 32);
static_assert(offsetof(Trade, d) == 64);
static_assert(offsetof(Trade, d) + offsetof(TradeData, trade_id) == 64);
static_assert(offsetof(Trade, d) + offsetof(TradeData, price) == 72);
static_assert(offsetof(Trade, d) + offsetof(TradeData, quantity) == 80);
static_assert(offsetof(Trade, d) + offsetof(TradeData, aggressor_side) == 88);
static_assert(offsetof(Trade, d) + offsetof(TradeData, trade_flags) == 90);
static_assert(offsetof(Trade, d) + offsetof(TradeData, native_condition) == 92);
static_assert(sizeof(Bbo) == 104);
static_assert(sizeof(BboData) == 40);
static_assert(offsetof(Bbo, d) == 64);
static_assert(offsetof(Bbo, d) + offsetof(BboData, bid_price) == 64);
static_assert(offsetof(Bbo, d) + offsetof(BboData, ask_quantity) == 88);
static_assert(offsetof(Bbo, d) + offsetof(BboData, bid_count) == 96);
static_assert(sizeof(BookReset) == 80);
static_assert(sizeof(FeedStatus) == 80);
static_assert(sizeof(FeedGap) == 96);
static_assert(sizeof(Heartbeat) == 80);
static_assert(sizeof(MboEvent) == 112);
static_assert(sizeof(MboData) == 48);
static_assert(offsetof(MboEvent, d) == 64);
static_assert(offsetof(MboEvent, d) + offsetof(MboData, order_id) == 64);
static_assert(offsetof(MboEvent, d) + offsetof(MboData, priority) == 72);
static_assert(offsetof(MboEvent, d) + offsetof(MboData, price) == 80);
static_assert(offsetof(MboEvent, d) + offsetof(MboData, quantity) == 88);
static_assert(offsetof(MboEvent, d) + offsetof(MboData, executed_quantity) == 96);
static_assert(offsetof(MboEvent, d) + offsetof(MboData, side) == 104);
static_assert(offsetof(MboEvent, d) + offsetof(MboData, action) == 105);
static_assert(offsetof(MboEvent, d) + offsetof(MboData, native_flags) == 106);
static_assert(sizeof(RawMessageHeader) == 48);
static_assert(sizeof(RawPacketHeader) == 40);
static_assert(sizeof(NativeEventHeader) == 40);

#undef REVOMD_POD

} // namespace revomd
