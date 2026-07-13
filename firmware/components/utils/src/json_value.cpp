#include "json_value.hpp"

#include <cstdlib>

namespace nova {
namespace {

constexpr std::size_t kMaxDepth = 32;

struct Cursor {
    const char* text;
    std::size_t len;
    std::size_t pos;
};

bool at_end(const Cursor& c) { return c.pos >= c.len; }
char peek(const Cursor& c) { return at_end(c) ? '\0' : c.text[c.pos]; }

void skip_whitespace(Cursor& c) {
    while (!at_end(c)) {
        const char ch = c.text[c.pos];
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            break;
        }
        ++c.pos;
    }
}

bool consume(Cursor& c, char expected) {
    if (peek(c) != expected) {
        return false;
    }
    ++c.pos;
    return true;
}

bool consume_literal(Cursor& c, const char* literal, std::size_t literal_len) {
    if (c.pos + literal_len > c.len) {
        return false;
    }
    for (std::size_t i = 0; i < literal_len; ++i) {
        if (c.text[c.pos + i] != literal[i]) {
            return false;
        }
    }
    c.pos += literal_len;
    return true;
}

// Codifica um code point (ate' 0xFFFF, sem par substituto) em UTF-8.
void append_utf8(std::string& out, unsigned code_point) {
    if (code_point <= 0x7F) {
        out.push_back(static_cast<char>(code_point));
    } else if (code_point <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
        out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
        out.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
}

bool hex_digit_value(char ch, unsigned& out) {
    if (ch >= '0' && ch <= '9') { out = static_cast<unsigned>(ch - '0'); return true; }
    if (ch >= 'a' && ch <= 'f') { out = static_cast<unsigned>(ch - 'a' + 10); return true; }
    if (ch >= 'A' && ch <= 'F') { out = static_cast<unsigned>(ch - 'A' + 10); return true; }
    return false;
}

bool parse_string_raw(Cursor& c, std::string& out) {
    if (!consume(c, '"')) {
        return false;
    }
    out.clear();
    while (true) {
        if (at_end(c)) {
            return false;  // unterminated string
        }
        const char ch = c.text[c.pos++];
        if (ch == '"') {
            return true;
        }
        if (ch != '\\') {
            out.push_back(ch);
            continue;
        }
        if (at_end(c)) {
            return false;
        }
        const char esc = c.text[c.pos++];
        switch (esc) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                if (c.pos + 4 > c.len) {
                    return false;
                }
                unsigned code_point = 0;
                for (int i = 0; i < 4; ++i) {
                    unsigned digit = 0;
                    if (!hex_digit_value(c.text[c.pos + static_cast<std::size_t>(i)], digit)) {
                        return false;
                    }
                    code_point = (code_point << 4) | digit;
                }
                c.pos += 4;
                append_utf8(out, code_point);  // sem suporte a par substituto
                break;
            }
            default:
                return false;  // escape inválido
        }
    }
}

bool parse_value(Cursor& c, JsonValue& out, std::size_t depth);

bool parse_number(Cursor& c, double& out) {
    const std::size_t start = c.pos;
    if (peek(c) == '-') {
        ++c.pos;
    }
    if (at_end(c) || c.text[c.pos] < '0' || c.text[c.pos] > '9') {
        return false;
    }
    while (!at_end(c) && c.text[c.pos] >= '0' && c.text[c.pos] <= '9') {
        ++c.pos;
    }
    if (!at_end(c) && c.text[c.pos] == '.') {
        ++c.pos;
        if (at_end(c) || c.text[c.pos] < '0' || c.text[c.pos] > '9') {
            return false;
        }
        while (!at_end(c) && c.text[c.pos] >= '0' && c.text[c.pos] <= '9') {
            ++c.pos;
        }
    }
    if (!at_end(c) && (c.text[c.pos] == 'e' || c.text[c.pos] == 'E')) {
        ++c.pos;
        if (!at_end(c) && (c.text[c.pos] == '+' || c.text[c.pos] == '-')) {
            ++c.pos;
        }
        if (at_end(c) || c.text[c.pos] < '0' || c.text[c.pos] > '9') {
            return false;
        }
        while (!at_end(c) && c.text[c.pos] >= '0' && c.text[c.pos] <= '9') {
            ++c.pos;
        }
    }
    const std::string token(c.text + start, c.pos - start);
    out = std::strtod(token.c_str(), nullptr);
    return true;
}

bool parse_array(Cursor& c, JsonValue& out, std::size_t depth) {
    if (!consume(c, '[')) {
        return false;
    }
    out = JsonValue{};
    out.array_.clear();
    skip_whitespace(c);
    if (consume(c, ']')) {
        out.type_ = JsonType::Array;
        return true;
    }
    while (true) {
        skip_whitespace(c);
        JsonValue element{};
        if (!parse_value(c, element, depth + 1)) {
            return false;
        }
        out.array_.push_back(std::move(element));
        skip_whitespace(c);
        if (consume(c, ',')) {
            continue;
        }
        if (consume(c, ']')) {
            break;
        }
        return false;
    }
    out.type_ = JsonType::Array;
    return true;
}

bool parse_object(Cursor& c, JsonValue& out, std::size_t depth) {
    if (!consume(c, '{')) {
        return false;
    }
    out = JsonValue{};
    out.object_keys_.clear();
    out.object_values_.clear();
    skip_whitespace(c);
    if (consume(c, '}')) {
        out.type_ = JsonType::Object;
        return true;
    }
    while (true) {
        skip_whitespace(c);
        std::string key;
        if (!parse_string_raw(c, key)) {
            return false;
        }
        skip_whitespace(c);
        if (!consume(c, ':')) {
            return false;
        }
        skip_whitespace(c);
        JsonValue member{};
        if (!parse_value(c, member, depth + 1)) {
            return false;
        }
        out.object_keys_.push_back(std::move(key));
        out.object_values_.push_back(std::move(member));
        skip_whitespace(c);
        if (consume(c, ',')) {
            continue;
        }
        if (consume(c, '}')) {
            break;
        }
        return false;
    }
    out.type_ = JsonType::Object;
    return true;
}

bool parse_value(Cursor& c, JsonValue& out, std::size_t depth) {
    if (depth > kMaxDepth) {
        return false;
    }
    skip_whitespace(c);
    const char ch = peek(c);
    if (ch == '{') {
        return parse_object(c, out, depth);
    }
    if (ch == '[') {
        return parse_array(c, out, depth);
    }
    if (ch == '"') {
        std::string s;
        if (!parse_string_raw(c, s)) {
            return false;
        }
        out = JsonValue{};
        out.type_ = JsonType::String;
        out.string_ = std::move(s);
        return true;
    }
    if (consume_literal(c, "true", 4)) {
        out = JsonValue{};
        out.type_ = JsonType::Bool;
        out.bool_ = true;
        return true;
    }
    if (consume_literal(c, "false", 5)) {
        out = JsonValue{};
        out.type_ = JsonType::Bool;
        out.bool_ = false;
        return true;
    }
    if (consume_literal(c, "null", 4)) {
        out = JsonValue{};
        out.type_ = JsonType::Null;
        return true;
    }
    double number = 0.0;
    if (parse_number(c, number)) {
        out = JsonValue{};
        out.type_ = JsonType::Number;
        out.number_ = number;
        return true;
    }
    return false;
}

}  // namespace

const JsonValue* JsonValue::find(const std::string& key) const {
    if (type_ != JsonType::Object) {
        return nullptr;
    }
    for (std::size_t i = 0; i < object_keys_.size(); ++i) {
        if (object_keys_[i] == key) {
            return &object_values_[i];
        }
    }
    return nullptr;
}

Result<JsonValue> parse_json(const std::string& text) {
    Cursor cursor{text.data(), text.size(), 0};
    JsonValue root{};
    if (!parse_value(cursor, root, 0)) {
        return Result<JsonValue>::failure(
            Status::error(StatusCode::InvalidArgument, "json parse failed"));
    }
    skip_whitespace(cursor);
    if (!at_end(cursor)) {
        return Result<JsonValue>::failure(
            Status::error(StatusCode::InvalidArgument, "trailing bytes after json value"));
    }
    return Result<JsonValue>::success(std::move(root));
}

}  // namespace nova
