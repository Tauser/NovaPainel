#pragma once

#include "status.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace nova {

// Parser JSON minimo e proprio: só o subconjunto que os providers precisam
// (objeto, array, número, string, bool, null). Existe para que o parsing dos
// providers compile e rode identico no host (host_check.sh) e no ESP-IDF --
// nenhuma das duas toolchains tem cJSON disponível sem depender de um
// idf.py build real (ver ADR-0007: fixtures obrigatórias exigem parsing
// coberto em CI, não só no alvo).
enum class JsonType : uint8_t {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object,
};

class JsonValue {
public:
    JsonType type() const { return type_; }
    bool is_object() const { return type_ == JsonType::Object; }
    bool is_array() const { return type_ == JsonType::Array; }
    bool is_number() const { return type_ == JsonType::Number; }
    bool is_string() const { return type_ == JsonType::String; }

    // 0.0 se não for número. Para os providers, campos numéricos vindos como
    // string (ex.: AwesomeAPI) são responsabilidade do chamador tentar
    // as_string() + strtod, não deste parser.
    double as_number() const { return type_ == JsonType::Number ? number_ : 0.0; }
    const std::string& as_string() const { return string_; }

    std::size_t size() const {
        return type_ == JsonType::Array ? array_.size()
             : type_ == JsonType::Object ? object_keys_.size() : 0;
    }
    const JsonValue* at(std::size_t index) const {
        return (type_ == JsonType::Array && index < array_.size()) ? &array_[index] : nullptr;
    }
    // Busca case-sensitive, só no nível deste objeto (sem recursão).
    const JsonValue* find(const std::string& key) const;

    // Representação interna. parse_json() é a única forma suportada de
    // construir um valor; estes campos são públicos só para os helpers
    // recursivos do parser (mesmo .cpp) preencherem sem uma cadeia de
    // `friend` por função -- não é uma API de builder para uso externo.
    // object_ usa dois vetores paralelos (não vector<pair<string,JsonValue>>)
    // porque pair<K,V> embute V inline: exigiria JsonValue completo no ponto
    // desta declaração, e ela ainda está sendo definida (tipo recursivo).
    JsonType type_{JsonType::Null};
    double number_{0.0};
    bool bool_{false};
    std::string string_{};
    std::vector<JsonValue> array_{};
    std::vector<std::string> object_keys_{};
    std::vector<JsonValue> object_values_{};
};

// Falha (InvalidArgument) em JSON malformado, truncado ou aninhado demais
// (limite de profundidade -- proteção de stack em embarcado). Nunca lança
// exceção nem tem comportamento indefinido em payload malicioso.
Result<JsonValue> parse_json(const std::string& text);

}  // namespace nova
