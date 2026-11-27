// A small JSON reader and writer for our own data files (the story file the
// importer writes). This is the MW1 rebuild's (mech1-re) src/Support/Json.*
// at fc77568, namespace aside and numbers read without std::from_chars
// (Json.cpp says why); that one is derived from the Crescent Hawk's Inception
// rebuild, src/formats/json.*, 258975d, and checked against its d30d26c (the
// same limits and messages); kept in step with it, so a fix in one can be
// ported to the other. Enough of JSON for those files: objects, arrays,
// strings, numbers, true/false/null. Strings are kept as bytes: \u0000-ÿ
// become one byte each, so the game's control codes and its 8-bit characters
// survive a round trip.
//
// Strict (as Inception's d30d26c): a duplicate key, a trailing
// comma, a raw control character in a string, a \u escape that isn't four
// hex digits, a number outside JSON's grammar, and nesting deeper than
// kJsonMaxDepth are errors, and every error names its line and column.
#pragma once

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace revenge::support {

struct JsonError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Arrays and objects nested deeper than this are refused (a hostile or broken
// file must not exhaust the stack).
inline constexpr std::size_t kJsonMaxDepth = 256;

struct Json {
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json, std::less<>>;
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value;

    bool is_object() const { return std::holds_alternative<Object>(value); }
    bool is_array() const { return std::holds_alternative<Array>(value); }
    bool is_string() const { return std::holds_alternative<std::string>(value); }
    bool is_number() const { return std::holds_alternative<double>(value); }
    // These throw JsonError if the value is another kind.
    const Object& object() const;
    const Array& array() const;
    const std::string& string() const;
    double number() const;
    // A member of an object, or null if it has none.
    const Json* find(std::string_view key) const;
};

// Throws JsonError naming the line and column (from 1, the column in bytes)
// of the problem: "JSON at line L, column C: why".
Json parse_json(std::string_view text);

// `bytes` as a JSON string literal, quotes included: control characters and
// bytes 0x7f-0xff are written as \u00XX, so the file stays plain ASCII.
std::string json_quote(std::string_view bytes);

}  // namespace revenge::support
