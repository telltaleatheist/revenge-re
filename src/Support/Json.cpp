#include "Json.h"

#include <algorithm>
#include <locale>
#include <sstream>
#include <cstdio>

namespace revenge::support {

const Json::Object& Json::object() const {
    if (auto* o = std::get_if<Object>(&value)) return *o;
    throw JsonError("JSON: expected an object");
}
const Json::Array& Json::array() const {
    if (auto* a = std::get_if<Array>(&value)) return *a;
    throw JsonError("JSON: expected an array");
}
const std::string& Json::string() const {
    if (auto* s = std::get_if<std::string>(&value)) return *s;
    throw JsonError("JSON: expected a string");
}
double Json::number() const {
    if (auto* n = std::get_if<double>(&value)) return *n;
    throw JsonError("JSON: expected a number");
}
const Json* Json::find(std::string_view key) const {
    const auto& o = object();
    const auto it = o.find(key);
    return it == o.end() ? nullptr : &it->second;
}

namespace {

bool is_digit(char c) { return c >= '0' && c <= '9'; }
int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

class Parser {
public:
    explicit Parser(std::string_view t) : t_(t) {}

    Json document() {
        Json v = value();
        space();
        if (i_ != t_.size()) fail("text after the end");
        return v;
    }

private:
    [[noreturn]] void fail(const std::string& why) {
        // The line and column from 1, the column counted in bytes (as Inception's).
        std::size_t line = 1, column = 1;
        for (std::size_t k = 0; k < i_ && k < t_.size(); ++k) {
            if (t_[k] == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
        }
        throw JsonError("JSON at line " + std::to_string(line) + ", column " + std::to_string(column) + ": " + why);
    }
    void space() {
        while (i_ < t_.size() && (t_[i_] == ' ' || t_[i_] == '\t' || t_[i_] == '\n' || t_[i_] == '\r')) ++i_;
    }
    bool take(char c) {
        space();
        if (i_ < t_.size() && t_[i_] == c) {
            ++i_;
            return true;
        }
        return false;
    }
    void expect(char c) {
        if (!take(c)) fail(std::string("expected '") + c + "'");
    }
    bool word(std::string_view w) {
        if (t_.substr(i_, w.size()) != w) return false;
        i_ += w.size();
        return true;
    }
    // Arrays and objects: one level deeper while their members are read.
    void enter() {
        if (++depth_ > kJsonMaxDepth) {
            --i_;  // at the bracket
            fail("nested deeper than " + std::to_string(kJsonMaxDepth));
        }
    }

    Json value() {
        space();
        if (i_ >= t_.size()) fail("unexpected end");
        const char c = t_[i_];
        if (c == '{') return object();
        if (c == '[') return array();
        if (c == '"') return Json{string()};
        if (word("true")) return Json{true};
        if (word("false")) return Json{false};
        if (word("null")) return Json{nullptr};
        if (c == '-' || is_digit(c)) return number();
        fail("unexpected character");
    }

    Json object() {
        expect('{');
        enter();
        Json::Object o;
        if (!take('}')) {
            do {
                space();
                if (i_ >= t_.size() || t_[i_] != '"') fail("expected a key");  // a trailing comma lands here
                const std::size_t key_at = i_;
                std::string key = string();
                if (o.count(key)) {
                    i_ = key_at;
                    fail("duplicate key");
                }
                expect(':');
                o.emplace(std::move(key), value());
            } while (take(','));
            expect('}');
        }
        --depth_;
        return Json{std::move(o)};
    }

    Json array() {
        expect('[');
        enter();
        Json::Array a;
        if (!take(']')) {
            do {
                space();
                if (i_ < t_.size() && t_[i_] == ']') fail("a trailing comma");
                a.push_back(value());
            } while (take(','));
            expect(']');
        }
        --depth_;
        return Json{std::move(a)};
    }

    std::string string() {
        ++i_;  // the opening quote
        std::string out;
        for (;;) {
            if (i_ >= t_.size()) fail("unfinished string");
            const char c = t_[i_++];
            if (c == '"') return out;
            if (static_cast<unsigned char>(c) < 0x20) {
                --i_;
                fail("control character in a string (write it as \\u00XX)");
            }
            if (c != '\\') {
                out += c;
                continue;
            }
            if (i_ >= t_.size()) fail("unfinished escape");
            const char e = t_[i_++];
            switch (e) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            case 'u': {
                if (i_ + 4 > t_.size()) fail("short \\u escape");
                int code = 0;
                for (std::size_t k = 0; k < 4; ++k) {
                    const int d = hex_digit(t_[i_ + k]);
                    if (d < 0) fail("\\u needs four hex digits");
                    code = code * 16 + d;
                }
                if (code > 0xff) fail("characters above \\u00ff are not used in game text");
                out += static_cast<char>(code);
                i_ += 4;
                break;
            }
            default: --i_; fail("unknown escape");
            }
        }
    }

    // JSON's grammar exactly: -?(0|[1-9][0-9]*)(.[0-9]+)?([eE][+-]?[0-9]+)?, read without the C locale's help.
    Json number() {
        const std::size_t start = i_;
        if (t_[i_] == '-') ++i_;
        if (i_ < t_.size() && t_[i_] == '0') {
            ++i_;
        } else if (i_ < t_.size() && is_digit(t_[i_])) {
            while (i_ < t_.size() && is_digit(t_[i_])) ++i_;
        } else {
            {
            i_ = start;
            fail("bad number");
        }
        }
        if (i_ < t_.size() && t_[i_] == '.') {
            ++i_;
            if (i_ >= t_.size() || !is_digit(t_[i_])) {
            i_ = start;
            fail("bad number");
        }
            while (i_ < t_.size() && is_digit(t_[i_])) ++i_;
        }
        if (i_ < t_.size() && (t_[i_] == 'e' || t_[i_] == 'E')) {
            ++i_;
            if (i_ < t_.size() && (t_[i_] == '+' || t_[i_] == '-')) ++i_;
            if (i_ >= t_.size() || !is_digit(t_[i_])) {
            i_ = start;
            fail("bad number");
        }
            while (i_ < t_.size() && is_digit(t_[i_])) ++i_;
        }
        if (i_ < t_.size() && is_digit(t_[i_])) {
            i_ = start;
            fail("a number with a leading zero");
        }
        // The grammar is checked above; the classic locale's stream reads it,
        // as std::from_chars for a double needs macOS 26 and the build runs on 12.
        double d = 0;
        std::istringstream in(std::string(t_.substr(start, i_ - start)));
        in.imbue(std::locale::classic());
        in >> d;
        if (in.fail() || !in.eof()) {
            i_ = start;
            fail("bad number");
        }
        return Json{d};
    }

    std::string_view t_;
    std::size_t i_ = 0;
    std::size_t depth_ = 0;
};

}  // namespace

Json parse_json(std::string_view text) { return Parser(text).document(); }

std::string json_quote(std::string_view bytes) {
    std::string out = "\"";
    for (const char ch : bytes) {
        const auto c = static_cast<unsigned char>(ch);
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\r') out += "\\r";
        else if (c == '\n') out += "\\n";
        else if (c < 0x20 || c >= 0x7f) {
            char buf[8];
            std::snprintf(buf, sizeof buf, "\\u%04x", c);
            out += buf;
        } else out += char(c);
    }
    return out + "\"";
}

}  // namespace revenge::support
