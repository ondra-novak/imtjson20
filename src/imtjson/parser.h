#pragma once

#include "value.h"
#include "common.h"

#include <variant>
namespace json {

template<typename T>
concept ValuePreprocessor = requires(T x, Value v) {
    {x(v)} -> std::convertible_to<Value>;
};



///Parser
/** Parser can be used to parse one JSON object. To start
 * parsing other object, you need to create a new instance
 *
 * @tparam Fn function which is called for every finished JSON value,
 * which can be transformed before it is stored.
 */


template<ValuePreprocessor Fn, Format format>
class Parser {
public:

    ///Construct parser
    /**
     * Construct parser with default preprocessor, or with preprocessor
     * initialized using default constructor
     */
    constexpr Parser();

    ///Construct parser pass preprecosser function
    /**
     * @param preprocFn preprocessor function.
     */
    constexpr Parser(Fn preprocFn);


    ///Write some data to the parser
    /**
     * @param text
     * @retval true need more data
     * @retval false parsing is done, you can get result/error
     */
    bool write(std::string_view text);

    ///Retrieve error status
    /**
     * @retval true parsing has been stopped because parse error. To determine, where
     * error happened, you can call get_unprocessed_data() to retrieve which data left
     * unprocessed and find out, where error occured
     * @return false no error reported / parsing is still in progress
     */
    bool is_error() const;


    ///Retrieve parsed result
    /**
     * @return returns parsed object. Returns undefined, if parse error.
     *
     * @note If parsing is still pending, the return value is undefined. It is always
     * a valid object, but returns content of variable which isused during parsing to
     * carry results between levels.
     */
    Value get_result();


    ///Retrieve unprocessed data
    /**
     * @return returns unprocessed data, the returned string is part of string
     * passed to the write().
     *
     * The function returns valid result only when parsing is done. Otherwise it
     * returns empty string. If empty string is returned after parsing
     * is done, it means, that all data has been processed
     */
    std::string_view get_unprocessed_data() const;

protected:

    //Reading new value, detect type
    struct DetectType {};

    //Reading string
    struct StateString {
        bool _escape = false;
        std::string _data;
    };

    //Reading number
    struct StateNumber {
        std::string _data;
    };

    struct StateDoubleNumber {
        std::string _data;
    };

    struct StateBinNumber {
        unsigned char size;
        bool negative = false;
        std::uint64_t accum = 0;
    };

    //Reading array
    struct StateArray {
        std::vector<Value> _data;
    };

    //Reading object
    struct StateObject {
        bool _reading_key = true;
        Key key;
        std::vector<KeyValue> _data;
    };
    //Checking token
    struct StateCheck {
        std::string_view what;
        Value result;
        std::size_t pos = 0;
    };

    struct StateBinString {
        bool is_number;
        std::size_t sz = 0;
        std::string data = {};
    };

    struct StateBinArray {
        std::size_t sz = 0;
        std::vector<Value> data;
    };
    struct StateBinObject {
        bool _reading_key = false;
        Key key;
        std::vector<KeyValue> data;
        std::size_t sz = 0;
    };

    using StateText = std::variant<DetectType, StateCheck, StateString, StateNumber, StateArray, StateObject>;
    using StateBin =  std::variant<DetectType, StateDoubleNumber, StateBinNumber, StateBinString, StateBinArray, StateBinObject>;
    using State = std::conditional_t<format == Format::text, StateText, StateBin>;

    Fn _preproc;
    std::vector<State> _state;

    std::string_view::iterator _pos = {};
    std::string_view::iterator _end = {};

    bool do_parse_cycle();

    Value _result;
    bool _is_error = false;

    bool parse_state(DetectType &);
    bool parse_state(StateString &st);
    bool parse_state(StateArray &st);
    bool parse_state(StateObject &st);
    bool parse_state(StateCheck &st);
    bool parse_state(StateNumber &st);
    bool parse_state(StateDoubleNumber &st);
    bool parse_state(StateBinNumber &st);
    bool parse_state(StateBinString &st);
    bool parse_state(StateBinArray &st);
    bool parse_state(StateBinObject &st);
    bool finish_state(DetectType &, const Value &v);
    bool finish_state(StateString &st, const Value &v);
    bool finish_state(StateArray &st, const Value &v);
    bool finish_state(StateObject &st, const Value &v);
    bool finish_state(StateCheck &st, const Value &v);
    bool finish_state(StateNumber &st, const Value &v);
    bool finish_state(StateDoubleNumber &st, const Value &v);
    bool finish_state(StateBinNumber &st, const Value &v);
    bool finish_state(StateBinString &st, const Value &v);
    bool finish_state(StateBinArray &st, const Value &v);
    bool finish_state(StateBinObject &st, const Value &v);


};

struct ParserEmptyPreprocesor {
    const Value &operator()(const Value &v) const {return v;}
};


template<ValuePreprocessor Fn>
Parser(Fn) -> Parser<Fn, Format::text>;

Parser() -> Parser<ParserEmptyPreprocesor, Format::text>;

using BinaryParser = Parser<ParserEmptyPreprocesor, Format::binary>;


template<typename Iter>
inline bool is_valid_json_number(Iter beg, Iter end) {
    if (beg == end) {
        return false;
    }


    if (*beg == '-') {
        ++beg;
        if (beg == end) {
            return false;
        }
    }

    if (std::equal(beg,end,infinity.begin())) {
        return true;
    }

    if (*beg == '0') {
        ++beg;
    } else {
        if (!std::isdigit(*beg)) {
            return false;
        }
        while (beg != end && std::isdigit(*beg)) {
            ++beg;
        }
    }

    if (beg != end && *beg == '.') {
        ++beg;
        if (beg == end || !std::isdigit(*beg)) {
            return false;
        }
        while (beg != end && std::isdigit(*beg)) {
            ++beg;
        }
    }

    if (beg != end && (*beg == 'e' || *beg == 'E')) {
        ++beg;

        if (beg != end && (*beg == '+' || *beg == '-')) {
            ++beg;
        }

        if (beg == end || !std::isdigit(*beg)) {
            return false;
        }

        while (beg != end && std::isdigit(*beg)) {
            ++beg;
        }
    }

    return beg == end;
}



template<ValuePreprocessor Fn, Format format>
inline constexpr Parser<Fn, format>::Parser():_state{DetectType()} {
}

template<ValuePreprocessor Fn, Format format>
inline constexpr Parser<Fn, format>::Parser(Fn preprocFn)
    :_preproc(std::move(preprocFn))
    ,_state{DetectType()}
{
}


template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::write(std::string_view text) {
    _pos = text.begin();
    _end = text.end();
    while (_pos != _end)  {
        if (!do_parse_cycle()) return false;
    }
    return !_state.empty();
}


template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::is_error() const {
    return _is_error;
}

template<ValuePreprocessor Fn, Format format>
inline Value Parser<Fn, format>::get_result() {
    return _is_error?Value():_result;
}

template<ValuePreprocessor Fn, Format format>
inline std::string_view Parser<Fn, format>::get_unprocessed_data() const {
    return std::string_view(_pos, std::distance(_pos, _end));
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::do_parse_cycle() {
    if (_state.empty()) return false;
    bool need_more = std::visit([&](auto &st){
        return parse_state(st);
    },_state.back());
    while (!need_more) {
        if (_is_error) return false;
        _state.pop_back();
        if (_state.empty()) return false;
        need_more = std::visit([&](auto &st){
            return finish_state(st, _result);
        }, _state.back());
    }
    return true;
}


template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::parse_state(DetectType&) {
    if (_pos == _end) return true;
    if constexpr(format == Format::text) {
        while (std::isspace(*_pos)) {
            ++_pos;
            if (_pos == _end) return true;
        }
        switch (*_pos) {
            case '[': _state.push_back(StateArray{});
                      ++_pos;
                      break;
            case '{': _state.push_back(StateObject{});
                      ++_pos;
                      break;
            case '"': _state.push_back(StateString{});
                      ++_pos;
                      break;
            case 't': _state.push_back(StateCheck{"true",true});
                      break;
            case 'f': _state.push_back(StateCheck{"false",false});
                      break;
            case 'n': _state.push_back(StateCheck{"null",nullptr});
                      break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '-':
            case '+': _state.push_back(StateNumber{});
                      break;
            default:
                    _is_error = true;
                    return false;
        }
        return true;
    } else {
        unsigned char type = *_pos++;
        unsigned char maj = type & BinaryType::mask;
        unsigned char sz = (type & BinaryType::size_mask) + 1;
        switch (maj) {
            case BinaryType::simple: {
                switch(type) {
                    case BinaryType::null: _result = _preproc(nullptr);return false;
                    case BinaryType::bool_true: _result = _preproc(true);return false;
                    case BinaryType::bool_false: _result = _preproc(false);return false;
                    case BinaryType::double_number:
                        _state.push_back(StateDoubleNumber());
                        break;
                    default: _result = _preproc(Value());return false;
                }
                break;
            }
            case BinaryType::p_number:
                _state.push_back(StateBinNumber{sz, false});
                break;
            case BinaryType::n_number:
                _state.push_back(StateBinNumber{sz, true});
                break;
            case BinaryType::string:
                _state.push_back(StateBinString{false});
                _state.push_back(StateBinNumber{sz, false});
                break;
            case BinaryType::string_number:
                _state.push_back(StateBinString{true});
                _state.push_back(StateBinNumber{sz, false});
                break;
            case BinaryType::array:
                 _state.push_back(StateBinArray());
                 _state.push_back(StateBinNumber{sz, false});
                break;
            case BinaryType::object:
                _state.push_back(StateBinObject());
                _state.push_back(StateBinNumber{sz, false});
                break;
            default:
                _is_error = true;
                return false;
        }
        return true;
    }

}

namespace _details {

inline int hex_to_int(char hex) {
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    } else if (hex >= 'A' && hex <= 'F') {
        return hex - 'A' + 10;
    } else if (hex >= 'a' && hex <= 'f') {
        return hex - 'a' + 10;
    }
    return 0;
}

}
template<typename Iter, typename OutIter>
OutIter decode_json_string(Iter beg, Iter end, OutIter output) {
    for (auto it = beg; it != end; ++it) {
        if (*it == '\\') {
            ++it;
            if (it == end) return output;
            char c = *it;
            switch (c) {
                case '"':
                case '\\':
                case '/':
                    *output = c;
                    break;
                case 'b':
                    *output = '\b';
                    break;
                case 'f':
                    *output = '\f';
                    break;
                case 'n':
                    *output = '\n';
                    break;
                case 'r':
                    *output = '\r';
                    break;
                case 't':
                    *output = '\t';
                    break;
                case 'u': {
                    ++it;
                    int codepoint = 0;
                    for (int i = 0; i < 4 && it != end; ++i) {
                        codepoint = (codepoint << 4) | _details::hex_to_int(*it);
                        ++it;
                    }
                    if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                        if (it == end || *it != '\\') return output;
                        ++it;
                        if (it == end || *it != 'u') return output;
                        ++it;
                        int second_codepoint = 0;
                        for (int i = 0; i < 4 && it != end; ++i) {
                            second_codepoint = (second_codepoint << 4) | _details::hex_to_int(*it);
                            ++it;
                        }
                        codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (second_codepoint - 0xDC00);
                    }
                    --it;
                    if (codepoint <= 0x7F) {
                        *output = static_cast<char>(codepoint);
                    } else if (codepoint <= 0x7FF) {
                        *output++ = static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
                        *output = static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else if (codepoint <= 0xFFFF) {
                        *output++ = static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
                        *output++ = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        *output = static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else if (codepoint <= 0x10FFFF) {
                        *output++ = static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
                        *output++ = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                        *output++ = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        *output = static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                }
                    break;
                default:
                    break;
            }
        } else {
            *output = *it;
        }
        ++output;
    }
    return output;
}


template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::parse_state(StateString &st) {
    while (_pos != _end) {
        if (!st._escape) {
            if (*_pos == '"') {
                ++_pos;
                auto end = decode_json_string(st._data.begin(), st._data.end(), st._data.begin());
                st._data.resize(std::distance(st._data.begin(), end));
                _result = Value(st._data);
                return false;
            } else if (*_pos == '\\') {
                st._escape = true;
            }
        } else {
            st._escape = false;
        }
        st._data.push_back(*_pos);
        ++_pos;
    }
    return true;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::parse_state(StateArray &st) {
    while (_pos != _end) {
        if (!std::isspace(*_pos)) {
            char c = *_pos;
            switch (c) {
                case ',': if (!st._data.empty()) {
                            ++_pos;
                            _state.push_back(DetectType{});
                            return true;
                        }
                break;
                case ']': ++_pos;
                          _result = Value(std::move(st._data));
                          return false;
                default: if (st._data.empty()) {
                            _state.push_back(DetectType{});
                             return true;
                          }
                break;
            }
            _is_error = true;
            return false;
        }
        ++_pos;
    }
    return true;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::parse_state(StateObject &st) {
    while (_pos != _end) {
        if (!std::isspace(*_pos)) {
            char c = *_pos;
            switch (c) {
                case ',':  if (st._reading_key && !st._data.empty()) {
                                ++_pos;
                                _state.push_back(DetectType{});
                                return true;
                            }
                break;

                case ':':   if (!st._reading_key) {
                                ++_pos;
                                _state.push_back(DetectType{});
                                return true;
                            }
                break;

                case '}':   if (st._reading_key) {
                                ++_pos;
                                _result = Value(std::move(st._data));
                                return false;
                            }
                break;
                default:    if (st._reading_key && st._data.empty())  {
                                _state.push_back(DetectType{});
                                return true;;
                            }
                break;

            }
            _is_error = true;
            return false;
        } else {
            ++_pos;
        }
    }
    return true;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::parse_state(StateCheck &st) {
    while (_pos != _end) {
        if (st.what[st.pos] == *_pos) {
            ++st.pos;
            ++_pos;
            if (st.pos == st.what.size()) {
                _result = st.result;
                return false;
            }
        } else {
            _is_error = true;
            return false;
        }
    }
    return true;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::parse_state(StateNumber &st) {
    while (_pos != _end) {
        char c = *_pos;
        if (std::isdigit(c) || c=='+' || c=='-' || c == 'e' || c == 'E' || c == '.') {
            st._data.push_back(c);
            ++_pos;
        } else {
            if (is_valid_json_number(st._data.begin(), st._data.end())) {
                _result = Value(st._data, true);
                return false;
            }
        }
    }
    return true;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::finish_state(DetectType&, const Value &v) {
    _result = _preproc(v);
    return false;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::finish_state(StateString &,const Value &) {
    return false;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::finish_state(StateArray &st, const Value &v) {
    st._data.push_back(v);
    return true;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::finish_state(StateObject &st,const Value &v) {
    if (st._reading_key) {
        if (v.type() != Type::string) {
            _is_error = true;
            return false;
        }
        st.key = v;
        st._reading_key = false;
    } else {
        st._data.push_back(KeyValue{st.key, v});
        st._reading_key = true;
    }
    return true;
}

class ParseError: public std::exception {
public:

    ParseError(std::size_t at): _at(at) {}
    const char *what() const noexcept override {
        std::snprintf(msgbuff,sizeof(msgbuff), "%s %lu", parse_error_msg.data(), _at);
        return msgbuff;
    }
protected:
    static constexpr std::string_view parse_error_msg = "JSON parse error at:";
    mutable char msgbuff[parse_error_msg.size()+20];
    std::size_t _at;

};


template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::finish_state(StateCheck &, const Value &) {
    return false;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::parse_state(StateDoubleNumber &st) {
    while (_pos != _end) {
        st._data.push_back(*_pos++);
        if (st._data.size() >= 8) {
            double v;
            std::copy(st._data.begin(), st._data.end(), reinterpret_cast<char *>(&v));
            _result = v;
            return false;
        }
    }
    return true;;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::parse_state(StateBinNumber &st) {
    while (_pos != _end && st.size) {
        st.accum = st.accum << 8 | static_cast<unsigned char>(*_pos++);
        --st.size;
    }
    if (st.size) return true;
    if (st.negative) {
        _result = -static_cast<std::int64_t>(st.accum);
    } else {
        _result = static_cast<std::uint64_t>(st.accum);
    }
    return false;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::parse_state(StateBinString &st) {
    while (_pos != _end) {
        st.data.push_back(*_pos++);
        if (st.data.size() >= st.sz) {
            _result = Value(st.data, st.is_number);
            return false;
        }
    }
    return true;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::parse_state(StateBinArray &st) {
    _result = Value(st.data);
    return false;

}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::parse_state(StateBinObject &st) {
    _result = Value(st.data);
    return false;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::finish_state(StateNumber &, const Value &) {
    return false;
}



template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::finish_state(StateDoubleNumber &, const Value &) {
    return false;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::finish_state(StateBinNumber &,const Value &) {
    return false;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::finish_state(StateBinString &st, const Value &v) {
    st.sz = v.get();
    if (st.sz == 0) {
        _result = "";
        return false;
    }
    st.data.reserve(st.sz);
    return true;
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::finish_state(StateBinArray &st, const Value &v) {
    if (st.sz == 0) {
        st.sz = v.get();
        if (st.sz == 0) {
            _result = Type::array;
            return false;
        }
        st.data.reserve(st.sz);
        _state.push_back(DetectType());
        return true;
    } else {
        st.data.push_back(v);
        if (st.data.size() < st.sz) {
            _state.push_back(DetectType());
            return true;
        } else {
            _result = std::move(st.data);
            return false;
        }
    }
}

template<ValuePreprocessor Fn, Format format>
inline bool Parser<Fn, format>::finish_state(StateBinObject &st, const Value &v) {
    if (st.sz == 0) {
        st.sz = v.get();
        if (st.sz == 0) {
            _result = Type::object;
            return false;
        }
        st.data.reserve(st.sz);
        st._reading_key = true;
        _state.push_back(DetectType());
        return true;
    } else if (st._reading_key){
        if (v.type() != Type::string) {
            _is_error = true;
            return false;
        }
        st.key = v;
        st._reading_key = false;
        _state.push_back(DetectType());
        return true;
    } else {
        st.data.push_back(KeyValue{st.key,v});
        if (st.data.size() < st.sz) {
            st._reading_key = true;
            _state.push_back(DetectType());
            return true;
        } else {
            _result = std::move(st.data);
            return false;
        }
    }
}

inline Value parse(std::string_view text) {
    Parser p;
    if (!p.write(text)) {
        if (p.is_error()) {
            auto unproc = p.get_unprocessed_data();
            auto at = text.size() - unproc.size();
            throw ParseError(at);
        }
        return p.get_result();
    } else {
        throw ParseError(text.size());
    }

}


inline Value unbinarize(std::string_view bin) {
    BinaryParser p;
    if (!p.write(bin)) {
        if (p.is_error()) {
            auto unproc = p.get_unprocessed_data();
            auto at = bin.size() - unproc.size();
            throw ParseError(at);
        }
        return p.get_result();
    } else {
        throw ParseError(bin.size());
    }

}


}


