#include <revomd/wire.h>

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

using namespace revomd;

TEST_CASE("EventHeader defaults to the current schema version", "[wire]") {
  EventHeader h{};
  REQUIRE(h.schema_version == kSchemaVersion);
}

TEST_CASE("BookLevel is LevelUpdate", "[wire]") {
  BookLevel lv{};
  lv.price = 100;
  lv.side = Side::Bid;
  LevelUpdate as_level_update = lv;
  REQUIRE(as_level_update.price == 100);
  REQUIRE(as_level_update.side == Side::Bid);
}

TEST_CASE("MsgType registry values are stable", "[wire]") {
  // These values are persisted in every existing capture's EventHeader-bearing
  // frame. Changing them silently corrupts replay of old journals.
  REQUIRE(static_cast<std::uint16_t>(MsgType::Trade) == 100);
  REQUIRE(static_cast<std::uint16_t>(MsgType::SnapshotDepth) == 101);
  REQUIRE(static_cast<std::uint16_t>(MsgType::DiffDepth) == 102);
  REQUIRE(static_cast<std::uint16_t>(MsgType::BBO) == 106);
  REQUIRE(static_cast<std::uint16_t>(MsgType::MboAdd) == 200);
  REQUIRE(static_cast<std::uint16_t>(MsgType::InstrumentDefinition) == 300);
  REQUIRE(static_cast<std::uint16_t>(MsgType::RawPacket) == 400);
}

TEST_CASE("BookChunk tracks interleaved bid/ask levels", "[wire]") {
  BookDeltaBatch32 chunk{};
  chunk.num_bids = 2;
  chunk.num_asks = 1;
  chunk.num_levels = 3;
  chunk.levels[0] = LevelUpdate{100, 5, 0, Side::Bid, Action::Upsert};
  chunk.levels[1] = LevelUpdate{99, 3, 0, Side::Bid, Action::Upsert};
  chunk.levels[2] = LevelUpdate{101, 4, 0, Side::Ask, Action::Upsert};

  REQUIRE(chunk.num_levels == chunk.num_bids + chunk.num_asks);
  REQUIRE(is_valid(chunk.levels[0].side));
  REQUIRE(is_valid(chunk.levels[2].side));
}

TEST_CASE("InstrumentDefinition defaults to Unknown venue and asset class",
          "[wire]") {
  InstrumentDefinition def{};
  REQUIRE(def.venue == Venue::Unknown);
  REQUIRE(def.asset_class == AssetClass::Unknown);
}
