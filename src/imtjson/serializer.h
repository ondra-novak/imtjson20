#pragma once
#include "json.h"
#include <variant>
#include <type_traits>
#include <string>
#include <map>


namespace json {

template<typename T>
concept NumberType = std::is_arithmetic_v<T>;

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


    std::string _out_buff;
    std::vector<State> _stack;
    std::map<const AbstractCustomValue *, Value> _custom_values;

    void next();
    void render_value(const Value &v);
    void render_key(const Key &v);

    void render_item(const Container<Value> &v, Type );
    void render_item(const Container<KeyValue> &v, Type );
    template<NumberType T>
    void render_item(const T &v, Type );
    void render_item(bool v, Type);
    void render_item(std::string_view v, Type type);
    void render_item(std::nullptr_t, Type);
    void render_item(Undefined, Type);
    void render_item(const AbstractCustomValue &v, Type);

};


inline std::string_view Serializer::read() {
    _out_buff.clear();
    next();
    return _out_buff;
}

inline void Serializer::next() {
    if (_stack.empty()) return;
    auto &st = _stack.back();
    if (std::holds_alternative<Value>(st)) {
        Value v = std::get<Value>(st);
        _stack.pop_back();
        render_value(v);
    } else if (std::holds_alternative<StateObject>(st)) {
        StateObject &s = std::get<StateObject>(st);
        if (s.pos == s.end) {
            _out_buff.push_back('}');
            _stack.pop_back();
            next();
        } else {
            const KeyValue &kv = *s.pos;
            ++s.pos;
            if (!kv.value.defined()) {
                next();
                return;
            }
            _out_buff.push_back(',');
            render_key(kv.key);
            _out_buff.push_back(':');
            render_value(kv.value);
        }
    } else {
        StateArray &s = std::get<StateArray>(st);
        if (s.pos == s.end) {
            _out_buff.push_back(']');
            _stack.pop_back();
            next();
        } else {
            const Value &v = *s.pos;
            ++s.pos;
            if (!v.defined()) {
                next();
                return;
            }
            _out_buff.push_back(',');
            render_value(v);
        }
    }
}

inline void Serializer::render_value(const Value &v) {
    v.visit([&](const auto &item){
        render_item(item, v.type());
    });
}

inline void Serializer::render_key(const Key &v) {
    render_item(v.get_string(),Type::string);
}

inline void Serializer::render_item(const Container<Value> &v, Type ) {
    _out_buff.push_back('[');
    auto pos = v.begin();
    auto end = v.end();
    while (pos != end) {
        const Value &v = *pos;
        ++pos;
        if (v.defined()) {
            _stack.push_back(StateArray{pos, end});
            render_value(v);
            return;
        }
    }
    _out_buff.push_back(']');
}

inline void Serializer::render_item(const Container<KeyValue> &v, Type ) {
    _out_buff.push_back('{');
    auto pos = v.begin();
    auto end = v.end();
    while (pos != end) {
        const KeyValue &kv = *pos;
        ++pos;
        if (kv.value.defined()) {
            render_key(kv.key);
            _out_buff.push_back(':');
            _stack.push_back(StateObject{pos, end});
            render_value(kv.value);
            return;
        }
    }
    _out_buff.push_back('}');
}

inline void Serializer::render_item(bool v, Type ) {
    _out_buff.append(v?"true":"false");
}

inline void Serializer::render_item(std::string_view v, Type type) {
    if (type == Type::number) {
        _out_buff.append(v);
    } else {
        _out_buff.push_back('"');
        encode(v, [&](char c){_out_buff.push_back(c);});
        _out_buff.push_back('"');
    }
}

inline void Serializer::render_item(std::nullptr_t nullptr_t, Type ) {
    _out_buff.append("null");
}

inline void Serializer::render_item(Undefined undefined, Type ) {
    _out_buff.append("\"undefined\"");
}

template<NumberType T>
inline void json::Serializer::render_item(const T &v, Type type) {
    _out_buff.append(std::to_string(v));
}

inline void Serializer::render_item(const AbstractCustomValue &v, Type type) {
    auto iter = _custom_values.find(&v);
    if (iter == _custom_values.end()) {
        iter = _custom_values.emplace(&v, v.to_json()).first;
    }
    render_value(iter->second);
}



template<typename Fn>
inline void Serializer::encode(std::string_view text, Fn fn) {
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


}
