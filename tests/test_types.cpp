#include <revomd/types.h>

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

using namespace revomd;

TEST_CASE("Side validity", "[types]") {
  REQUIRE(is_valid(Side::Bid));
  REQUIRE(is_valid(Side::Ask));
  REQUIRE_FALSE(is_valid(static_cast<Side>(0)));
}

TEST_CASE("Action validity", "[types]") {
  REQUIRE(is_valid(Action::Upsert));
  REQUIRE(is_valid(Action::Delete));
  REQUIRE(is_valid(Action::Clear));
  REQUIRE_FALSE(is_valid(static_cast<Action>(99)));
}

TEST_CASE("Side value-initializes to the reserved-invalid zero", "[types]") {
  Side s{};
  REQUIRE_FALSE(is_valid(s));
  REQUIRE(static_cast<std::uint8_t>(s) == 0);
}

TEST_CASE("LevelUpdate value-initializes with a reserved-invalid side",
          "[types]") {
  LevelUpdate lv{};
  REQUIRE_FALSE(is_valid(lv.side));
  REQUIRE(lv.action == Action::Upsert);
}

TEST_CASE("RestingOrder defaults to a valid Bid side", "[types]") {
  RestingOrder order{};
  REQUIRE(is_valid(order.side));
  REQUIRE(order.side == Side::Bid);
}

TEST_CASE("QuoteLevel equality", "[types]") {
  QuoteLevel a{100, 10, 2};
  QuoteLevel b{100, 10, 2};
  QuoteLevel c{101, 10, 2};
  REQUIRE(a == b);
  REQUIRE_FALSE(a == c);
}

TEST_CASE("RestingOrder equality", "[types]") {
  RestingOrder a{1, Side::Bid, 100, 10};
  RestingOrder b{1, Side::Bid, 100, 10};
  RestingOrder c{2, Side::Bid, 100, 10};
  REQUIRE(a == b);
  REQUIRE_FALSE(a == c);
}
