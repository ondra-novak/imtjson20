#pragma once
#include "value.h"
#include "common.h"

#include <variant>
#include <type_traits>
#include <string>
#include <map>
#include <cmath>


namespace json {

template<typename T>
concept NumberType = std::is_arithmetic_v<T>;

template<typename T>
concept IntegralType = std::is_integral_v<T>;

template<Format format = Format::text>
class Serializer {
public:

    Serializer(Value v):_stack({v}) {}

    ///Read serialized content
    /**
     * You need to read serialized content until empty string is returned
     * @return part of serialized content. If empty string is returned, everything
     * is serialized
     */
    std::string_view read();


    template<typename Fn>
    static void encode(std::string_view text, Fn fn);

public:



protected:

    struct StateArray {
        Value::Iterator pos;
        Value::Iterator end;
    };
    struct StateObject {
        Value::KeyValueIterator pos;
        Value::KeyValueIterator end;
    };
    using State = std::variant<Value, StateObject, StateArray>;


    std::vector<char> _out_buff;
    std::vector<State> _stack;
    std::map<const AbstractCustomValue *, Value> _custom_values;

    void next();
    void render_value(const Value &v);
    void render_key(const Key &v);

    void render_item(const Container<Value> &v, Type );
    void render_item(const Container<KeyValue> &v, Type );
    template<IntegralType T>
    void render_item(const T &v, Type );
    void render_item(double v, Type );
    void render_item(bool v, Type);
    void render_item(std::string_view v, Type type);
    void render_item(std::nullptr_t, Type);
    void render_item(Undefined, Type);
    void render_item(const AbstractCustomValue &v, Type);

    void render_binary_type_size(unsigned char type, std::uint64_t size);
};


template<Format format>
inline std::string_view Serializer<format>::read() {
    _out_buff.clear();
    next();
    return std::string_view(_out_buff.data(),_out_buff.size());
}

template<Format format>
inline void Serializer<format>::next() {
    if (_stack.empty()) return;
    auto &st = _stack.back();
    if (std::holds_alternative<Value>(st)) {
        Value v = std::get<Value>(st);
        _stack.pop_back();
        render_value(v);
    } else if (std::holds_alternative<StateObject>(st)) {
        StateObject &s = std::get<StateObject>(st);
        if (s.pos == s.end) {
            if constexpr(format == Format::text) {
                _out_buff.push_back('}');
            }
            _stack.pop_back();
            next();
        } else {
            const KeyValue &kv = *s.pos;
            ++s.pos;
            if constexpr(format == Format::text) {
                if (!kv.value.defined()) {
                    next();
                    return;
                }
                _out_buff.push_back(',');
            }
            render_key(kv.key);
            if constexpr(format == Format::text) {
                _out_buff.push_back(':');
            }
            render_value(kv.value);
        }
    } else {
        StateArray &s = std::get<StateArray>(st);
        if (s.pos == s.end) {
            if constexpr(format == Format::text) {
                _out_buff.push_back(']');
            }
            _stack.pop_back();
            next();
        } else {
            const Value &v = *s.pos;
            ++s.pos;
            if constexpr(format == Format::text) {
                if (!v.defined()) {
                    next();
                    return;
                }
                _out_buff.push_back(',');
            }
            render_value(v);
        }
    }
}

template<Format format>
inline void Serializer<format>::render_value(const Value &v) {
    v.visit([&](const auto &item){
        render_item(item, v.type());
    });
}

template<Format format>
inline void Serializer<format>::render_key(const Key &v) {
    render_item(v.get_string(),Type::string);
}

template<Format format>
inline void Serializer<format>::render_item(const Container<Value> &v, Type ) {
    if constexpr(format == Format::text) {
        _out_buff.push_back('[');
    } else {
        render_binary_type_size(BinaryType::array, v.size());
    }
    auto pos = v.begin();
    auto end = v.end();
    while (pos != end) {
        const Value &v = *pos;
        ++pos;
        if (format == Format::binary || v.defined()) {
            _stack.push_back(StateArray{pos, end});
            render_value(v);
            return;
        }
    }
    if constexpr(format == Format::text) {
        _out_buff.push_back(']');
    }
}

template<Format format>
inline void Serializer<format>::render_item(const Container<KeyValue> &v, Type ) {
    if constexpr(format == Format::text) {
        _out_buff.push_back('{');
    } else {
        render_binary_type_size(BinaryType::object, v.size());
    }
    auto pos = v.begin();
    auto end = v.end();
    while (pos != end) {
        const KeyValue &kv = *pos;
        ++pos;
        if (format == Format::binary || kv.value.defined()) {
            render_key(kv.key);
            if constexpr(format == Format::text) {
                _out_buff.push_back(':');
            }
            _stack.push_back(StateObject{pos, end});
            render_value(kv.value);
            return;
        }
    }
    if constexpr(format == Format::text) {
        _out_buff.push_back('}');
    }
}

template<Format format>
inline void Serializer<format>::render_item(bool v, Type ) {
    if constexpr(format == Format::text) {
        auto sel = v?true_value:false_value;
        std::copy(sel.begin(), sel.end(), std::back_inserter(_out_buff));
    } else {
        _out_buff.push_back(v?BinaryType::bool_true:BinaryType::bool_false);
    }
}

template<Format format>
inline void Serializer<format>::render_item(std::string_view v, Type type) {
    if constexpr(format == Format::text) {
        if (type == Type::number) {
            std::copy(v.begin(), v.end(), std::back_inserter(_out_buff));
        } else {
            _out_buff.push_back('"');
            encode(v, [&](char c){_out_buff.push_back(c);});
            _out_buff.push_back('"');
        }
    } else {
        render_binary_type_size(type == Type::number?BinaryType::string_number:BinaryType::string, v.size());
        std::copy(v.begin(), v.end(), std::back_inserter(_out_buff));
    }
}

template<Format format>
inline void Serializer<format>::render_item(std::nullptr_t , Type ) {
    if constexpr(format == Format::text) {
        std::copy(null_value.begin(), null_value.end(), std::back_inserter(_out_buff));
    } else {
        _out_buff.push_back(BinaryType::null);
    }
}

template<Format format>
inline void Serializer<format>::render_item(Undefined , Type ) {
    if constexpr(format == Format::text) {
        render_item(nullptr, Type::null);
    }else {
        _out_buff.push_back(BinaryType::undefined);
    }
}


template<typename T, typename Iter>
Iter render_unsigned_number2(T val, Iter out) {
    if (val) {
        unsigned int dig = static_cast<unsigned int>(val%10);
        out = render_unsigned_number2(val/10, out);
        *out++ = dig+'0';
    }
    return out;
}


template<typename T, typename Iter>
Iter render_unsigned_number(T val, Iter out) {
    if (!val) {
        *out++ = '0';
        return out;
    } else {
        char buff[sizeof(T)*3];
        auto itr = std::rbegin(buff);
        while (val != 0) {
            unsigned int dig = static_cast<unsigned int>(val%10);
            val = val / 10;
            *itr++ = dig+'0';
        }
        auto chrs = std::distance(std::rbegin(buff), itr);
        return std::copy(std::end(buff)-chrs, std::end(buff), out);
    }

}


template<Format format>
template<IntegralType T>
inline void json::Serializer<format>::render_item(const T &v, Type t) {
    if constexpr(format == Format::text) {
        if constexpr(!std::is_unsigned_v<T>) {
            if (v < 0) {
                _out_buff.push_back('-');
                render_item(static_cast<std::make_unsigned_t<T> >(-v), t);
                return;
            }
        }
        render_unsigned_number(v, std::back_inserter(_out_buff));
    } else {
        unsigned char type;
        std::uint64_t val;
        if constexpr(!std::is_unsigned_v<T>) {
            if (v < 0) {
                type = BinaryType::n_number;
                val = static_cast<std::uint64_t>(-v);
            } else {
                type = BinaryType::p_number;
                val = static_cast<std::uint64_t>(v);
            }
        } else {
            type = BinaryType::p_number;
            val = static_cast<std::uint64_t>(v);
        }
        render_binary_type_size(type, val);
    }

}

template<Format format>
inline void Serializer<format>::render_item(const AbstractCustomValue &v, Type ) {
    auto iter = _custom_values.find(&v);
    if (iter == _custom_values.end()) {
        iter = _custom_values.emplace(&v, v.to_json()).first;
    }
    render_value(iter->second);
}

template<Format format>
inline void Serializer<format>::render_item(double v, Type ) {
    if constexpr(format == Format::text) {
        if (std::isnan(v)) {
            render_item(nullptr,Type::null);
            return;
        }
        if (!std::isfinite(v)) {
            _out_buff.push_back('"');
            if (v < 0) std::copy(neg_infinity.begin(), neg_infinity.end(), std::back_inserter(_out_buff));
            else std::copy(infinity.begin(), infinity.end(), std::back_inserter(_out_buff));
            _out_buff.push_back('"');
            return;
        }

        if (v < 0) {
            _out_buff.push_back('-');
            v = -v;
        }
        if (v < std::numeric_limits<double>::min()) {
            _out_buff.push_back('0');
            return;
        }
        double exp_f = std::log10(v);
        int exponent = static_cast<int>(std::floor(exp_f));
        if (exponent < -2 || exponent>8) {
            v = v / std::pow(10, exponent);
        } else {
            exponent = 0;
        }
        double ip;
        double fr = std::modf(v+std::numeric_limits<double>::epsilon(), &ip);
        render_item(static_cast<unsigned long>(ip), Type::number);
        if (fr > 1e-6) {
            _out_buff.push_back('.');
            for (int i = 0; i < 12 && fr > 1e-6; i++) {
                fr = std::modf(fr * 10, &ip);
                unsigned int n = static_cast<unsigned int>(ip)+'0';
                _out_buff.push_back(static_cast<char>(n));
            }
        }
        if (exponent) {
            _out_buff.push_back('e');
            if (exponent > 0) {
                _out_buff.push_back('+');
            }
            render_item(exponent, Type::number);
        }
    } else {
        _out_buff.push_back(BinaryType::double_number);
        std::string_view data (reinterpret_cast<const char *>(&v), sizeof(v));
        std::copy(data.begin(), data.end(), std::back_inserter(_out_buff));
    }

}

template<Format format>
template<typename Fn>
inline void Serializer<format>::encode(std::string_view text, Fn fn) {
    for (char c : text) {
          switch (c) {
              case '"':fn('\\');fn('"');break;
              case '\\':fn('\\');fn('\\');break;
              case '\b':fn('b');fn('\\');break;
              case '\f':fn('\\');fn('f');break;
              case '\n':fn('\\');fn('n');break;
              case '\r':fn('\\');fn('f');break;
              case '\t':fn('\\');fn('t');break;
              default:
                  if (c>= 0 && c < 0x20) {
                      char buffer[7];
                      std::snprintf(buffer, sizeof(buffer), "\\u%04X", static_cast<unsigned int>(c));
                      for (int i = 0; i < 6; ++i) fn(buffer[i]);
                  } else {
                      fn(c);
                  }
                  break;
          }
    }
}


inline std::string stringify(const Value &v) {
    std::string retval;
    Serializer ser(v);
    std::string_view part = ser.read();
    while (!part.empty()) {
        retval.append(part);
        part = ser.read();
    }
    return retval;
}

using BinarySerializer = Serializer<Format::binary>;

inline std::string binarize(const Value &v) {
    std::string retval;
    BinarySerializer ser(v);
    std::string_view part = ser.read();
    while (!part.empty()) {
        retval.append(part);
        part = ser.read();
    }
    return retval;
}


template<Format format>
inline void json::Serializer<format>::render_binary_type_size(unsigned char type, std::uint64_t size) {
    unsigned char count_bytes = 0;
    auto tmp = size;
    for (unsigned int i = 0; i < 8; ++i) {
        count_bytes += bool(tmp > 0);
        tmp>>=8;
    }
    count_bytes = std::max<unsigned char>(1, count_bytes);
    _out_buff.push_back(type | (count_bytes-1));
    while (count_bytes > 0) {
        --count_bytes;
        _out_buff.push_back((size >> (count_bytes * 8)) & 0xFF);
    }
}

}

