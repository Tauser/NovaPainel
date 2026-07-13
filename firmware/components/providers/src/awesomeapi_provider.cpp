#include "awesomeapi_provider.hpp"

#include "http_client.hpp"
#include "json_value.hpp"

#include <cstdlib>

namespace nova {
namespace {
constexpr const char* kUrl = "https://economia.awesomeapi.com.br/json/last/USD-BRL";

// AwesomeAPI devolve campos numéricos como string (ex.: "bid": "5.4321").
// Aceita número JSON também, por robustez a mudança de schema da API.
bool read_numeric_field(const JsonValue& object, const char* key, double& out) {
    const JsonValue* item = object.find(key);
    if (item == nullptr) {
        return false;
    }
    if (item->is_number()) {
        out = item->as_number();
        return true;
    }
    if (item->is_string() && !item->as_string().empty()) {
        char* end = nullptr;
        const double parsed = std::strtod(item->as_string().c_str(), &end);
        if (end == item->as_string().c_str()) {
            return false;  // no digits consumed: not a number
        }
        out = parsed;
        return true;
    }
    return false;
}
}  // namespace

Result<MarketState> AwesomeApiProvider::parse_forex_payload(const std::string& body) {
    const auto parsed = parse_json(body);
    if (!parsed.ok()) {
        return Result<MarketState>::failure(parsed.status());
    }
    if (!parsed.value().is_object()) {
        return Result<MarketState>::failure(
            Status::error(StatusCode::InvalidArgument, "awesomeapi payload is not an object"));
    }
    const JsonValue* pair = parsed.value().find("USDBRL");
    if (pair == nullptr || !pair->is_object()) {
        return Result<MarketState>::failure(
            Status::error(StatusCode::InvalidArgument, "awesomeapi payload missing USDBRL object"));
    }

    double bid = 0.0;
    if (!read_numeric_field(*pair, "bid", bid) || bid <= 0.0) {
        return Result<MarketState>::failure(
            Status::error(StatusCode::InvalidArgument, "awesomeapi payload missing/invalid bid"));
    }

    MarketState state{};
    state.usd_brl = bid;
    state.valid = true;
    return Result<MarketState>::success(state);
}

Result<MarketState> AwesomeApiProvider::fetch_forex() {
    HttpClient http_client;
    const auto response = http_client.get(kUrl);
    if (!response.ok()) {
        return Result<MarketState>::failure(response.status());
    }
    return parse_forex_payload(response.value().body);
}

}  // namespace nova
