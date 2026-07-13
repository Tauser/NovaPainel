#include "coingecko_provider.hpp"

#include "http_client.hpp"
#include "json_value.hpp"

namespace nova {
namespace {
constexpr const char* kUrl =
    "https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=bitcoin";
}

Result<MarketState> CoinGeckoProvider::parse_market_payload(const std::string& body) {
    const auto parsed = parse_json(body);
    if (!parsed.ok()) {
        return Result<MarketState>::failure(parsed.status());
    }
    if (!parsed.value().is_array() || parsed.value().size() == 0) {
        return Result<MarketState>::failure(
            Status::error(StatusCode::InvalidArgument, "coingecko payload missing market array"));
    }
    const JsonValue* first = parsed.value().at(0);
    if (first == nullptr || !first->is_object()) {
        return Result<MarketState>::failure(
            Status::error(StatusCode::InvalidArgument, "coingecko payload missing market object"));
    }
    const JsonValue* price = first->find("current_price");
    if (price == nullptr || !price->is_number() || price->as_number() <= 0.0) {
        return Result<MarketState>::failure(
            Status::error(StatusCode::InvalidArgument, "coingecko payload missing current_price"));
    }

    MarketState state{};
    state.btc_usd = price->as_number();
    state.valid = true;
    return Result<MarketState>::success(state);
}

Result<MarketState> CoinGeckoProvider::fetch_market() {
    HttpClient http_client;
    const auto response = http_client.get(kUrl);
    if (!response.ok()) {
        return Result<MarketState>::failure(response.status());
    }
    return parse_market_payload(response.value().body);
}

}  // namespace nova
