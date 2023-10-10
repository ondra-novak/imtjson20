#pragma once


#include <type_traits>
#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <vector>
#include <algorithm>

namespace json {

enum class Type : char {
    undefined,
    null,
    boolean,
    number,
    string,
    array,
    object
};

template<typename T, typename Result, typename ... Args>
concept InvokableResult = std::constructible_from<Result, decltype(std::declval<T>()(std::declval<Args>()...))>;

class RefCounted {
public:

    constexpr RefCounted()
        :_refcnt(std::is_constant_evaluated()*std::numeric_limits<long>::max()) {}

    void add_ref() const {
        _refcnt.fetch_add(1, std::memory_order_relaxed);
    }
    bool release_ref() const {
        if (_refcnt.fetch_sub(1, std::memory_order_release) <= 1) {
            _refcnt.load(std::memory_order_acquire);
            return true;
        }
        return false;
    }

    struct Deleter {
        template<typename T>
        void operator()(T *x) {
            if (x && x->release_ref()) delete x;
        }
    };

protected:
    mutable std::atomic<unsigned long> _refcnt;
};


template<typename T>
class Container;

template<typename T>
using PContainer = std::unique_ptr<Container<T>, RefCounted::Deleter>;

template<std::derived_from<RefCounted> T, typename ... Args>
inline auto make_refcnt(Args && ... args) {
    auto ptr = new T(std::forward<Args>(args)...);
    ptr->add_ref();
    return std::unique_ptr<T, RefCounted::Deleter>(ptr);
}

template<typename T>
inline auto share_ref(const std::unique_ptr<T, RefCounted::Deleter> &x) {
    auto ptr = x.get();
    x->add_ref();
    return std::unique_ptr<T, RefCounted::Deleter>(ptr);
}



template<typename T>
class Container: public RefCounted {
public:
    constexpr Container():_ptr(nullptr),_sz(0) {}

    ///iterator to begin of string
    constexpr const T *begin() const {return _ptr;}
    ///iterator to end of string
    constexpr const T *end() const {return _ptr+_sz;}

    constexpr const T *data() const {return _ptr;}
    constexpr std::size_t size() const {return _sz;}


    ///iterator to begin of string - string is mutable in this case
    constexpr T *begin() {return const_cast<T *>(this->_ptr);}
    ///iterator to end of string - string is mutable in this case
    constexpr T *end() {return  const_cast<T *>(this->_ptr)+this->_sz;}
    ///change size of the string (you are allowed only to shrink the size
    constexpr void set_size(const T *new_end) {
        auto newsz = new_end - begin();
        this->_sz = newsz;
    }

    virtual constexpr ~Container() {
        for (T &x:*this) x.~T();
    }

    Container(const Container &) = delete;
    Container &operator=(const Container &) = delete;

    constexpr bool operator==(const Container &other)  const {
        if (_sz != other._sz) return false;
        if (_ptr == other._ptr) return true;
        for (std::size_t i = 0; i < _sz; ++i) {
            if (_ptr[i] != other._ptr[i]) return false;
        }
        return true;
    }


    static PContainer<T> create(const T *ptr, std::size_t sz) {
        AllocInfo info = {sz};
        auto out = new(info) Container<T> (info,ptr);
        out->add_ref();
        return PContainer<T>(out);
    }
    static PContainer<T> create_move_in(T *ptr, std::size_t sz) {
        AllocInfo info = {sz};
        auto out = new(info) Container<T> (info,ptr);
        out->add_ref();
        return PContainer<T>(out);
    }
    static PContainer<T> create(std::size_t sz) {
        AllocInfo info = {sz};
        auto out = new(info) Container<T>(info);
        out->add_ref();
        return PContainer<T>(out);
    }
protected:
    const T *_ptr;
    std::size_t _sz;


    struct AllocInfo { // @suppress("Miss copy constructor or assignment operator")
        std::size_t sz;
        T *buffer = nullptr;
    };


    Container(AllocInfo &info):_ptr(info.buffer), _sz(info.sz) {
        for (auto &target: *this) std::construct_at(&target);
    }

    Container(AllocInfo &info, const T *source):_ptr(info.buffer), _sz(info.sz) {
        for (auto &target: *this) {
            std::construct_at(&target, *source);
            ++source;
        }
    }
    Container(AllocInfo &info, T *source):_ptr(info.buffer), _sz(info.sz) {
        for (auto &target: *this) {
            std::construct_at(&target, std::move(*source));
            ++source;
        }
    }


    void *operator new(std::size_t sz, AllocInfo &info) {
        std::size_t total_size = sz + sizeof(T)*info.sz;
        void *out = ::operator new(total_size);
        info.buffer = reinterpret_cast<T *>(reinterpret_cast<char *>(out) + sz);
        return out;
    }
    void operator delete(void *ptr, AllocInfo &) {
        ::operator delete(ptr);
    }

public:
    void operator delete(void *ptr, std::size_t) {
        ::operator delete(ptr);
    }
};


struct KeyValue;

enum class Storage : unsigned char {
    short_string_0 = 0,
    short_string_1 = 1,
    short_string_2 = 2,
    short_string_3 = 3,
    short_string_4 = 4,
    short_string_5 = 5,
    short_string_6 = 6,
    short_string_7 = 7,
    short_string_8 = 8,
    short_string_9 = 9,
    short_string_10 = 10,
    short_string_11 = 11,
    short_string_12 = 12,
    short_string_13 = 13,
    short_string_14 = 14,
    short_string_top = 15,
    short_number_0 = 16,
    short_number_1 = 17,
    short_number_2 = 18,
    short_number_3 = 19,
    short_number_4 = 20,
    short_number_5 = 21,
    short_number_6 = 22,
    short_number_7 = 23,
    short_number_8 = 24,
    short_number_9 = 25,
    short_number_10 = 26,
    short_number_11 = 27,
    short_number_12 = 28,
    short_number_13 = 29,
    short_number_14 = 30,
    undefined = 32,
    null = 33,
    bool_false = 34,
    bool_true = 35,
    int32 = 36,
    uint32 = 37,
    int64 = 38,
    uint64 = 39,
    dnum = 40,
    empty_array = 41,
    empty_object = 42,
    long_string = 43,
    long_number = 44,
    array = 45,
    object = 46,
    string_ref = 47,
    number_ref = 48,
    custom_type = 49

};


struct StorageBase {


    unsigned char type_size;
};

struct ShortStringStorage: StorageBase {
    char _data[15];
};

template<typename T>
struct DirectStorage: StorageBase {
    T _val;
};

class Undefined {};
class IsNumber {};
class Value;

constexpr std::string_view infinity="∞";
constexpr std::string_view neg_infinity="-∞";
constexpr std::string_view true_value="true";
constexpr std::string_view false_value="false";
constexpr std::string_view null_value="null";
constexpr std::string_view undefined_value="(undefined)";

///Interface to create custom values
/**
 * Custom value can be anything which you need to store as json::Value.
 * It is strongly recommended to keep value immutable.
 *
 * It is mandatory to associate the custom value with a json::Type. It can
 * affect how the value is accessed. The most meaningful types are string,array,object
 *
 * The custom value can be serialized, however it is required to generate standard json
 * value during serialization.
 */
class AbstractCustomValue: public RefCounted {
public:
    virtual constexpr ~AbstractCustomValue() {}

    ///Mandatory function
    /** Returns string representation of this value
     * @return string representation
     * */
    virtual std::string to_string() const = 0;
    ///Mandatory function
    /** Returns associated compatible type
     * @return type*/
    virtual Type type() const = 0;

    ///Optional, retrieve string content
    /**
     * @return string content. Default value is empty string
     */
    virtual std::string_view get_string() const {return {};}
    ///Optional, if the custom value acts as container, you can specify count of items
    /**
     * @return count of items in the container
     */
    virtual std::size_t size() const {return 0;}
    ///Optional, converts custom value to JSON structure
    /**
     * @return JSON structure, it if is possible. This function
     * is called during serialization. Default value is null
     */
    virtual Value to_json() const;
    ///Optional, retrieve item from container
    /**
     * @param index index of item 0 <= index < size().
     * @return item at given index. If the index is out of range, function must return undefined value
     *
     * @note function is mandatory in case, that custom value acts as an object, otherwise
     * it is impossible to enumerate all items (iterator doesn't work then)
     */
    virtual const Value &operator[](unsigned int index) const;
    ///Optional, retrieve item from container searched by a key
    /**
     * @param key key value
     * @return item at given key, if doesn't exists, function must return undefined
     */
    virtual const Value &operator[](const std::string_view &key) const;


    ///Optional, compare two custom values
    virtual bool operator==(const AbstractCustomValue &other) const {
        return this == &other;
    }


};


using PCustomValue = std::unique_ptr<const AbstractCustomValue, RefCounted::Deleter>;


#pragma pack(push, 1)


class Value {
public:


    constexpr Value();
    constexpr ~Value();

    constexpr Value(std::string_view str, bool is_number = false);
    constexpr Value(const char * str):Value(std::string_view(str)) {}

    constexpr Value(short val);
    constexpr Value(unsigned short val);
    constexpr Value(int val);
    constexpr Value(unsigned int val);
    constexpr Value(long val);
    constexpr Value(unsigned long val);
    constexpr Value(long long val);
    constexpr Value(unsigned long long val);
    constexpr Value(float val):Value(static_cast<double>(val)) {}
    constexpr Value(double val);
    constexpr Value(bool b);
    constexpr Value(std::nullptr_t);
    constexpr Value(Type type);
    Value(PCustomValue custom);


    Value(const std::vector<Value> &arr);
    Value(std::vector<Value> &&arr);
    template<size_t _Extent>
    Value(const std::span<const Value, _Extent> &arr);
    template<size_t _Extent>
    Value(std::span<Value, _Extent> &&arr);

    Value(const std::vector<KeyValue> &arr);
    Value(std::vector<KeyValue> &&arr);
    template<size_t _Extent>
    Value(const std::span<const KeyValue, _Extent> &arr);
    template<size_t _Extent>
    Value(std::span<KeyValue, _Extent> &&arr);

    Value(std::initializer_list<Value> list);

    ///Initialize container
    /**
     * @tparam Iter Iterator which returns items. Type of final container
     * depends on type of the value returned by the iterator. If
     * the iterator returns single value, such a Value, result is array. If
     * the iterator returns KeyValue, object is constructed
     * @param from
     * @param to
     */
    template<typename Iter>
    Value(Iter from, Iter to);

    ///Construct container by transforming values
    /**
     *
     * @tparam Iter source iterator
     * @param from from
     * @param to to
     * @param fn transform function, which takes one argument - item - and must
     * transform it to Value or KeyValue
     */
    template<typename Iter, typename TransformFn>
    Value(Iter from, Iter to, TransformFn fn);

    constexpr Value(const Value &other);
    constexpr Value(Value &&other);
    constexpr Value &operator=(const Value &other);
    constexpr Value &operator=(Value &&other);


    Value(PContainer<Value> v):_un{.array = v.release()},_storage(Storage::array) {}
    Value(PContainer<KeyValue> v):_un{.object = v.release()},_storage(Storage::object) {}


    template<typename Fn>
    constexpr decltype(auto) visit(Fn &&fn) const;

    constexpr Type type() const;

    const Value &operator[](const std::string_view &key) const;
    const Value &operator[](unsigned int index) const;

    ///Retrieve whether value is defined
    /**
     * @retval true value is defined
     * @retval false value is undefined
     */
    constexpr bool defined() const {return _storage != Storage::undefined;}
    ///Retrieves whether there is value (not undefined nor null)
    /**
     * @retval true value is available
     * @retval false value is either undefined or null
     */
    constexpr bool has_value() const {return _storage != Storage::undefined && _storage != Storage::null;}
    ///Retrieve boolean
    /**
     * @retval true value is boolean and it is set to true
     * @retval false otherwise
     */
    constexpr bool get_bool() const {return _storage == Storage::bool_true;}
    ///Retrieves whether value is container (can be iterated)
    /**
     * @retval true is container
     * @retval false is not container
     */
    constexpr bool is_container() const {return _storage == Storage::array || _storage == Storage::object;}
    ///retrieve value
    constexpr short get_short() const;
    ///retrieve value
    constexpr unsigned short get_unsigned_short() const;
    ///retrieve value
    constexpr int get_int() const;
    ///retrieve value
    constexpr unsigned int get_unsigned_int() const;
    ///retrieve value
    constexpr long get_long() const;
    ///retrieve value
    constexpr unsigned long get_unsigned_long() const;
    ///retrieve value
    constexpr long long get_long_long() const;
    ///retrieve value
    constexpr unsigned long long get_unsigned_long_long() const;
    ///retrieve value
    constexpr double get_double() const;
    ///retrieve value
    constexpr float get_float() const;
    ///retrieve string value
    /**
     * @return string value
     * @note the object must hold a string value. It cannot convert number to string.
     * @see to_string
     */
    constexpr std::string_view get_string() const;
    ///Determines whether stored value is empty container
    /**
     * @retval true value is empty container or it is not container
     * @retval false value is container which is not empty
     */
    constexpr bool empty() const;
    ///Returns count of items in the container
    /**
     * @return count of items in the container, if this is not container, returns zero
     */
    constexpr std::size_t size() const;
    ///Retrieve custom value if stored in it
    /**
     * @return pointer to custom value, if the object doesn't hold custom value, returns
     * nullptr
     */
    PCustomValue get_custom() const;

    ///Retrieve value if stored, or default value if not
    constexpr bool get(bool defval) const;
    ///Retrieve value if stored, or default value if not
    constexpr short get(short defval) const;
    ///Retrieve value if stored, or default value if not
    constexpr unsigned short get(unsigned short defval) const;
    ///Retrieve value if stored, or default value if not
    constexpr int get(int defval) const;
    ///Retrieve value if stored, or default value if not
    constexpr unsigned int get(unsigned int defval) const;
    ///Retrieve value if stored, or default value if not
    constexpr long get(long defval) const;
    ///Retrieve value if stored, or default value if not
    constexpr unsigned long get(unsigned long defval) const;
    ///Retrieve value if stored, or default value if not
    constexpr long long get(long long defval) const;
    ///Retrieve value if stored, or default value if not
    constexpr unsigned long long get(unsigned long long defval) const;
    ///Retrieve value if stored, or default value if not
    constexpr double get(double defval) const;
    ///Retrieve value if stored, or default value if not
    constexpr float get(float defval) const;
    ///Retrieve value if stored, or default value if not
    constexpr std::string_view get(const std::string_view &defval) const;

    ///Retrieve direct reference to stored array
    /**
     * @return reference to stored array, if array is not stored, reference to an
     * empty array is returned.
     *
     * @note reference is valid until original object is destroyed
     */
    constexpr const Container<Value> &get_array() const;
    ///Retrieve direct reference to stored object
    /**
     * @return reference to stored array, if object is not stored, reference to an
     * empty object is returned.
     *
     * @note reference is valid until original object is destroyed
     */
    constexpr const Container<KeyValue> &get_object() const;

    ///Converts content to string
    /**
     * @return string representing the content
     * @note currently only boolean, number, string, null and undefined can
     * be converted to string which contains string reprezentation of their values.
     * Containers emits just single generic string
     */
    std::string to_string() const;

    ///Iterator
    /** Allows to iterate the container and read values. It is
     * possible to iterate stored array or stored object. If object
     * is iterated, the keys are not available
     *
     * To access key-value items, obtain iterator through method keys()
     */
    class Iterator;

    ///KeyValue iterator
    /**
     * Helps to iterate KeyValue containers
     */
    using KeyValueIterator = const KeyValue *;

    ///Retrieve iterator pointing at the begin of the container
    /**
     * @return iterator at the begin. If the object doesn't hold container, it
     * returns end()
     */
    constexpr Iterator begin() const;
    ///Retrieve iterator pointing at the end of the container
    /**
     * @return iterator at the end. If the object doesn't hold container, it
     * returns begin()
     */
    constexpr Iterator end() const;

    ///Helper class to access stored object in it
    class KeyAccess;

    ///Retrieve a helper object, which allows to work with key-values
    constexpr KeyAccess keys() const;

    ///Create a custom value
    /**
     * @tparam T type - must be specified, must implement AbstractCustomValue
     * @param args arguments of constuctor of the T
     * @return custom value
     */
    template<std::derived_from<AbstractCustomValue> T, typename ... Args>
    static Value custom(Args && ... args) {
        auto ptr = new T(std::forward<Args>(args)...);
        ptr->add_ref();
        return Value(PCustomValue(ptr));
    }

    ///Helper class which automatically converts Value to required type
    class GetHelper;
    ///Retrieve helper class which can be used to automatically convert Value to required type
    /**
     * The Value class doesn't declare conversion operators. This is implemented
     * separatedly by GetHelper. You can use get() function to achieve this automatic
     * conversion
     *
     * @code
     * int number = value.get()   // int number = value.get_int();
     * @endcode
     *
     * @return get helper
     */
    GetHelper get() const;

    ///Merge two stored objects
    /**
     * @param changes A Value which contains stored object which keys is to be merged
     * into current object
     * @return reference to current object (merged in place)
     *
     * @note This and argument should be objects. Equal keys are replaced by values
     * from 'changes'. If value is 'undefined' it is erased.
     */
    Value &merge_keys(const Value &changes);
    ///Sets keys in the current value
    /**
     * @param items list of key-value pairs. Existing keys are replaced. If they
     * are replaced by undefined value, they are actually erased.
     * @return reference to this
     *
     * @code
     * Value v = {
     *     {"deleted",42},
     *     {"replaced","hello}
     * };
     * v.set_keys({
     *     {"new",123},
     *     {"replaced","hello"},
     *     {"deleted",undefined}
     *  })
     *
     */
    Value &set_keys(std::initializer_list<std::pair<std::string_view, Value> > items);
    Value &insert(Iterator at, std::initializer_list<Value> data);
    Value &insert(Iterator at, Value data);
    Value &insert(Iterator at, Iterator beg, Iterator end);
    Value &erase(Iterator from, Iterator to);
    Value &append(Value array);
    Value &append(std::initializer_list<Value> data);
    Value slice(Iterator from, Iterator to);
    Value splice(Iterator from, Iterator to, std::initializer_list<Value> items);
    template<typename Iter>
    Value splice(Iterator from, Iterator to, Iter new_from, Iter new_to);


    template<std::invocable<Value> Fn>
    Value filter(Fn fn);
    template<std::invocable<KeyValue> Fn>
    Value filter(Fn fn);
    template<InvokableResult<Value, Value> Fn>
    Value map(Fn fn);
    template<InvokableResult<KeyValue, KeyValue> Fn>
    Value map(Fn fn);
    template<InvokableResult<Value, KeyValue> Fn>
    Value map(Fn fn);
    template<InvokableResult<KeyValue, Value> Fn>
    Value map(Fn fn);

    constexpr bool operator==(const Value &other) const;

    constexpr Storage get_storage() const {return _storage;}

protected:

    struct StringRef { // @suppress("Miss copy constructor or assignment operator")
        const char *ptr;
        std::uint32_t sz;
    };

    union Un { // @suppress("Miss copy constructor or assignment operator")
        double dnum;
        std::int32_t int32;
        std::uint32_t uint32;
        std::int64_t int64;
        std::uint64_t uint64;
        StringRef str_ref;
        char short_str[15];
        const Container<char> *long_str;
        const Container<Value> *array;
        const Container<KeyValue> *object;
        const AbstractCustomValue *custom;

    };
    Un _un;
    Storage _storage;



    static constexpr void release(Value &v);
    static constexpr void add_ref(Value &v);

    template<typename X>
    void init_array(const X &x);
    template<typename X>
    void init_array_move(X &&x);

    template<typename X>
    void init_object(const X &x);
    template<typename X>
    void init_object_move(X &&x);

    static PContainer<KeyValue> sort_object(PContainer<KeyValue> ptr);

    template<typename Num>
    constexpr void init_integral(Num num) {
        if constexpr(sizeof(num) <= 4) {
            if constexpr(std::is_unsigned_v<Num>) {
                _un.uint32 = static_cast<std::uint32_t>(num);
                _storage = Storage::uint32;
            } else {
                _un.int32 = static_cast<std::int32_t>(num);
                _storage = Storage::int32;
            }
        } else {
            if constexpr(std::is_unsigned_v<Num>) {
                _un.uint64 = static_cast<std::uint64_t>(num);
                _storage = Storage::uint64;
            } else {
                _un.int64 = static_cast<std::int64_t>(num);
                _storage = Storage::int64;
            }
        }
    }
};

class Key {
public:

    constexpr Key() = default;
    constexpr Key(const Value &v):_str(v) {}
    constexpr Key(std::string_view val):_str(val) {}
    Key(const std::string &val):_str(std::string_view(val)) {}
    constexpr operator std::string_view() const {return _str.get_string();}
    operator std::string() const {return std::string(_str.get_string());}
    constexpr const char *c_str() const {return _str.get_string().data();}
    constexpr std::size_t size() const {return _str.get_string().size();}
    constexpr bool empty() const {return _str.get_string().empty();}
    constexpr std::string_view get_string() const {return _str.get_string();}
    constexpr std::string_view get() const {return _str.get_string();}
    constexpr auto compare(const std::string_view &other) const {return _str.get_string().compare(other);}
    constexpr auto operator<=>(const Key &other) const {return compare(other);}
    constexpr bool operator==(const Key &other) const  = default;


protected:
    Value _str;
};

#pragma pack(pop)

std::ostream& operator << (std::ostream& stream, const Key &key) {
    return stream << key.get_string();
}

struct KeyValue {
    Key key;
    Value value;

    KeyValue() = default;
    KeyValue(std::string_view key, const Value &v):key(key),value(v) {}

    constexpr bool operator==(const KeyValue &other) const {
        return key == other.key && value == other.value;
    }
};

class Array: public Value {
public:
    constexpr Array():Value(Type::array) {}
    Array(std::initializer_list<Value> items):Value(std::span<const Value>(items.begin(), items.size())) {}
};
class Object: public Value {
public:
    constexpr Object():Value(Type::object) {}
    Object(std::initializer_list<KeyValue>  items):Value(std::span<const KeyValue>(items.begin(), items.size())) {}
};

template<typename Fn>
inline constexpr decltype(auto) json::Value::visit(Fn &&fn) const  {
    switch (_storage) {
        default: return fn(std::string_view(_un.short_str, static_cast<int>(_storage) & 0xF));
        case Storage::undefined: return fn(Undefined());
        case Storage::null: return fn(nullptr);
        case Storage::bool_false: return fn(false);
        case Storage::bool_true: return fn(true);
        case Storage::int32: return fn(_un.int32);
        case Storage::uint32: return fn(_un.uint32);
        case Storage::int64: return fn(_un.int64);
        case Storage::uint64: return fn(_un.uint64);
        case Storage::dnum: return fn(_un.dnum);
        case Storage::empty_array: return fn(Container<Value>());
        case Storage::empty_object: return fn(Container<KeyValue>());
        case Storage::array: return fn(*_un.array);
        case Storage::object: return fn(*_un.object);
        case Storage::long_number:
        case Storage::long_string:  return fn(std::string_view(_un.long_str->data(), _un.long_str->size()));
        case Storage::number_ref:
        case Storage::string_ref: return fn(std::string_view(_un.str_ref.ptr, _un.str_ref.sz));
        case Storage::custom_type: return fn(*_un.custom);

    }
}

inline constexpr Value::Value():_un{0},_storage{Storage::undefined} {}

inline constexpr void Value::release(Value &v) {
    switch(v._storage) {
        case Storage::array: if (v._un.array->release_ref())
                                    delete v._un.array;
                              break;
        case Storage::object: if (v._un.object->release_ref())
                                    delete v._un.object;
                              break;
        case Storage::long_number:
        case Storage::long_string: if (v._un.long_str->release_ref())
                                    delete v._un.long_str;
                              break;
        case Storage::custom_type: if (v._un.custom->release_ref())
                                    delete v._un.custom;
                              break;
        default:
            break;
    }

}

inline constexpr void Value::add_ref(Value &v) {
    switch(v._storage) {
        case Storage::array: v._un.array->add_ref();
                              break;
        case Storage::object: v._un.object->add_ref();
                              break;
        case Storage::long_number:
        case Storage::long_string: v._un.long_str->add_ref();
                              break;
        case Storage::custom_type: v._un.custom->add_ref();
                              break;
        default:
            break;
    }

}


inline constexpr Value::~Value() {
    release(*this);
}


constexpr Value undefined = {};

inline constexpr Value::Value(const Value &other):_un(other._un),_storage(other._storage) {
    add_ref(*this);
}

inline constexpr Value::Value(Value &&other):_un(other._un),_storage(other._storage) {
    other._storage = Storage::undefined;
}

inline constexpr Value &Value::operator=(const Value &other) {
    if (this != &other) {
        release(*this);
        _un = other._un;
        _storage = other._storage;
        add_ref(*this);
    }
    return *this;
}

inline constexpr Value &Value::operator=(Value &&other) {
    if (this != &other) {
        release(*this);
        _un = other._un;
        _storage = other._storage;
        other._storage = Storage::undefined;
    }
    return *this;
}

inline constexpr Value::Value(std::string_view str, bool is_number)
{
    if (str.size() < 15) {
        _un.short_str[14] = 0; // activate variant
        auto iter = std::copy(str.begin(), str.end(),std::begin(_un.short_str));
        std::fill(iter, std::end(_un.short_str), '\0');
        _storage = static_cast<Storage>(static_cast<int>(
                is_number?Storage::short_number_0:Storage::short_string_0
            ) | static_cast<unsigned char>(str.size()));
    } else if (std::is_constant_evaluated()) {
        _un.str_ref.ptr = str.data();
        _un.str_ref.sz = static_cast<std::uint32_t>(str.size());
        _storage = is_number?Storage::number_ref:Storage::string_ref;
    } else {
        _un.long_str = Container<char>::create(str.data(),str.size()).release();
        _storage = is_number?Storage::long_number:Storage::long_string;
    }
}


inline constexpr Type Value::type() const  {
    switch (_storage) {
        default: return static_cast<Storage>(static_cast<int>(_storage) & 0xF0) == Storage::short_number_0?Type::number:Type::string;
        case Storage::undefined: return Type::undefined;
        case Storage::null: return Type::null;
        case Storage::bool_false:
        case Storage::bool_true: return Type::boolean;
        case Storage::int32:
        case Storage::uint32:
        case Storage::int64:
        case Storage::uint64:
        case Storage::dnum: return Type::number;
        case Storage::empty_array:
        case Storage::array: return Type::array;
        case Storage::empty_object:
        case Storage::object: return Type::object;
        case Storage::long_number:
        case Storage::number_ref: return Type::number;
        case Storage::long_string:
        case Storage::string_ref: return Type::string;
        case Storage::custom_type: return _un.custom->type();
    }

}

template<typename X>
inline void Value::init_array(const X &arr) {
    if (arr.empty()) {
        _storage = Storage::empty_array;
    } else {
        _un.array = Container<Value>::create(arr.data(), arr.size()).release();
        _storage = Storage::array;
    }
}
template<typename X>
inline void Value::init_array_move(X &&arr) {
    if (arr.empty()) {
        _storage = Storage::empty_array;
    } else {
        _un.array = Container<Value>::create_move_in(arr.data(), arr.size()).release();;
        _storage = Storage::array;
    }
}

inline Value::Value(const std::vector<Value> &arr) {init_array(arr);}
inline Value::Value(std::vector<Value> &&arr) {init_array_move(std::move(arr));}
template<size_t _Extent>
inline Value::Value(const std::span<const Value, _Extent> &arr) {init_array(arr);}
template<size_t _Extent>
inline Value::Value(std::span<Value, _Extent> &&arr) {init_array_move(std::move(arr));}

template<typename X>
inline void Value::init_object(const X &obj) {
    if (obj.empty()) {
        _storage = Storage::empty_object;
    } else {
        _un.object= sort_object(Container<KeyValue>::create(obj.data(), obj.size())).release();
        _storage = Storage::object;
    }

}
template<typename X>
inline void Value::init_object_move(X &&obj) {
    if (obj.empty()) {
        _storage = Storage::empty_object;
    } else {
        _un.object = sort_object(Container<KeyValue>::create_move_in(obj.data(), obj.size())).release();
        _storage = Storage::object;
    }
}

inline Value::Value(PCustomValue custom)
    :_un{.custom = custom.release()},_storage(Storage::custom_type)
{

}


inline PContainer<KeyValue> Value::sort_object(PContainer<KeyValue> ptr) {
    std::sort(ptr->begin(),ptr->end(),[&](const KeyValue &a, const KeyValue &b) {
        return a.key.get_string() < b.key.get_string();
    });
    return ptr;
}


inline Value::Value(const std::vector<KeyValue> &arr) {init_object(arr);}
inline Value::Value(std::vector<KeyValue> &&arr) {init_object_move(std::move(arr));}
template<size_t _Extent>
inline Value::Value(const std::span<const KeyValue, _Extent> &arr) {init_object(arr);}
template<size_t _Extent>
inline Value::Value(std::span<KeyValue, _Extent> &&arr) {init_object_move(std::move(arr));}

inline constexpr Value::Value(double val):_un{.dnum = val},_storage{Storage::dnum} {
    _storage=Storage::dnum;
}

inline constexpr Value::Value(short val) {init_integral(val);}
inline constexpr Value::Value(unsigned short val) {init_integral(val);}
inline constexpr Value::Value(int val) {init_integral(val);}
inline constexpr Value::Value(unsigned int val) {init_integral(val);}
inline constexpr Value::Value(long val) {init_integral(val);}
inline constexpr Value::Value(unsigned long val) {init_integral(val);}
inline constexpr Value::Value(long long val) {init_integral(val);}
inline constexpr Value::Value(unsigned long long val) {init_integral(val);}

inline constexpr Value::Value(bool b):_un{0},_storage{b?Storage::bool_true:Storage::bool_false} {}
inline constexpr Value::Value(std::nullptr_t):_un{0},_storage{Storage::null} {}
inline constexpr Value::Value(Type type):_un{0} {
    switch (type) {
        default:
        case Type::undefined: _storage = Storage::undefined; break;
        case Type::string: _storage = Storage::short_string_0; break;
        case Type::number: _storage = Storage::int32; break;
        case Type::null: _storage = Storage::null; break;
        case Type::boolean: _storage = Storage::bool_false; break;
        case Type::array: _storage = Storage::empty_array; break;
        case Type::object: _storage = Storage::empty_object; break;
    }
}

inline const Value &Value::operator [](const std::string_view &key) const {
    return visit([key](const auto &item) -> const Value &{
        using T = std::decay_t<decltype(item)>;
        if constexpr(std::is_same_v<T, Container<KeyValue> >) {

            auto iter = std::lower_bound(item.begin(), item.end(), key, [&](const auto &a, const auto &b){
                std::string_view sa;
                std::string_view sb;
                using TA = std::decay_t<decltype(a)>;
                using TB = std::decay_t<decltype(b)>;
                if constexpr(std::is_same_v<TA, std::string_view>) {
                    sa = a;
                } else {
                    sa = a.key.get_string();
                }
                if constexpr(std::is_same_v<TB, std::string_view>) {
                    sb = b;
                } else {
                    sb = b.key.get_string();
                }
                return sa < sb;
            });
            if (iter == item.end() || iter->key.get_string() != key) return undefined;
            return iter->value;
        } else if constexpr(std::is_same_v<T, AbstractCustomValue>){
            return item[key];
        } else {
            return undefined;;
        }
    });
}

inline const Value &Value::operator [](unsigned int index) const {
    return visit([index](const auto &item) -> const Value &{
        using T = std::decay_t<decltype(item)>;
        if constexpr(std::is_same_v<T, Container<KeyValue> >) {
            if (index >= item.size()) return undefined;
            else return item.data()[index].value;
        } else if constexpr(std::is_same_v<T, Container<Value> >) {
            if (index >= item.size()) return undefined;
            else return item.data()[index];
        } else if constexpr(std::is_same_v<T, AbstractCustomValue>){
            return item[index];
        } else {
            return undefined;
        }
    });
}

inline constexpr short Value::get_short() const {
    return static_cast<short>(get_int());
}
inline constexpr unsigned short Value::get_unsigned_short() const {
    return static_cast<unsigned short>(get_int());
}
inline constexpr int Value::get_int() const {
    return visit([](const auto &a) ->int{
       using A = std::decay_t<decltype(a)>;
       if constexpr(std::is_arithmetic_v<A>) {return static_cast<int>(a);}
       else if constexpr(std::is_same_v<A, std::string_view>) {return static_cast<int>(std::strtol(a.data(),nullptr,10));}
       else return 0;
    });
}
inline constexpr unsigned int Value::get_unsigned_int() const {
    return visit([](const auto &a) ->unsigned int{
        using A = std::decay_t<decltype(a)>;
        if constexpr(std::is_arithmetic_v<A>) {return static_cast<unsigned int>(a);}
        else if constexpr(std::is_same_v<A, std::string_view>) {return static_cast<unsigned int>(std::strtoul(a.data(),nullptr,10));}
        else return 0;
    });
}
inline constexpr long Value::get_long() const {
    return visit([](const auto &a) ->long{
       using A = std::decay_t<decltype(a)>;
       if constexpr(std::is_arithmetic_v<A>) {return static_cast<long>(a);}
       else if constexpr(std::is_same_v<A, std::string_view>) {return static_cast<long>(std::strtol(a.data(),nullptr,10));}
       else return 0;
    });
}
inline constexpr unsigned long Value::get_unsigned_long() const {
    return visit([](const auto &a) ->unsigned long{
        using A = std::decay_t<decltype(a)>;
        if constexpr(std::is_arithmetic_v<A>) {return static_cast<unsigned long>(a);}
        else if constexpr(std::is_same_v<A, std::string_view>) {return static_cast<unsigned long>(std::strtoul(a.data(),nullptr,10));}
        else return 0;
    });
}
inline constexpr long long Value::get_long_long() const {
    return visit([](const auto &a) ->long long{
       using A = std::decay_t<decltype(a)>;
       if constexpr(std::is_arithmetic_v<A>) {return static_cast<long long>(a);}
       else if constexpr(std::is_same_v<A, std::string_view>) {return std::strtoll(a.data(),nullptr,10);}
       else return 0;
    });
}
inline constexpr unsigned long long Value::get_unsigned_long_long() const {
    return visit([](const auto &a) ->unsigned long long{
        using A = std::decay_t<decltype(a)>;
        if constexpr(std::is_arithmetic_v<A>) {return static_cast<unsigned long long>(a);}
        else if constexpr(std::is_same_v<A, std::string_view>) {return std::strtoull(a.data(),nullptr,10);}
        else return 0;
    });

}
inline constexpr double Value::get_double() const {
    return visit([](const auto &a) ->double{
        using A = std::decay_t<decltype(a)>;
        if constexpr(std::is_arithmetic_v<A>) {return static_cast<double>(a);}
        else if constexpr(std::is_same_v<A, std::string_view>) {
            if (a.empty()) return std::numeric_limits<double>::signaling_NaN();
             char *end = nullptr;
            double r= std::strtod(a.data(),&end);
            if (end != a.data()+a.size()) {
                if (a == neg_infinity) {
                    return -std::numeric_limits<double>::infinity();
                } else if (a == infinity) {
                    return std::numeric_limits<double>::infinity();
                } else {
                    return std::numeric_limits<double>::signaling_NaN();
                }
            }
            return r;
        }
        else return std::numeric_limits<double>::signaling_NaN();
    });
}
inline constexpr float Value::get_float() const {
    return static_cast<float>(get_double());
}
inline constexpr std::string_view Value::get_string() const {
    return visit([](const auto &a) ->std::string_view{
        using A = std::decay_t<decltype(a)>;
        if constexpr(std::is_same_v<A, std::string_view>) {return a;}
        else if constexpr(std::is_same_v<A, bool>) {return a?true_value:false_value;}
        else if constexpr(std::is_same_v<A, AbstractCustomValue>) {return a.get_string();}
        else if constexpr(std::is_null_pointer_v<A>) {return null_value;}
        else if constexpr(std::is_same_v<A, Undefined>) {return undefined_value;}
        else return {};
    });
}

constexpr bool Value::empty() const {
    return visit([](const auto &a) ->bool{
        using A = std::decay_t<decltype(a)>;
        if constexpr(std::is_same_v<A, Container<Value> >
                     || std::is_same_v<A, Container<KeyValue> >
                     || std::is_same_v<A, AbstractCustomValue>) {
            return a.size() == 0;
        } else {
            return true;
        }
    });
}
constexpr std::size_t Value::size() const {
    return visit([](const auto &a) -> std::size_t{
        using A = std::decay_t<decltype(a)>;
        if constexpr(std::is_same_v<A, Container<Value> >
                  || std::is_same_v<A, Container<KeyValue> >
                  || std::is_same_v<A, AbstractCustomValue>) {
            return a.size();
        } else {
            return 0;
        }
    });

}

std::string Value::to_string() const {
    return visit([](const auto &a) -> std::string {
        using A = std::decay_t<decltype(a)>;
        if constexpr (std::is_arithmetic_v<A>) {return std::to_string(a);}
        else if constexpr (std::is_same_v<A, std::string_view>) {return std::string(a);}
        else if constexpr(std::is_same_v<A, Container<Value> >) {
            return "[array]";
        }
        else if constexpr(std::is_same_v<A, Container<KeyValue> >) {
            return "{object}";
        }
        else if constexpr(std::is_same_v<A, Undefined>) {
            return std::string(undefined_value);
        }
        else if constexpr(std::is_same_v<A, AbstractCustomValue>) {
            return a.to_string();
        }
        else if constexpr(std::is_null_pointer_v<A>) {
            return std::string(null_value);
        } else {
            static_assert(std::is_same_v<A, bool>);
            return std::string(a?true_value:false_value);
        }
    });
}




inline const Value &AbstractCustomValue::operator [](unsigned int ) const {
    return undefined;
}

inline Value AbstractCustomValue::to_json() const {
    return Value(nullptr);
}

inline const Value &AbstractCustomValue::operator [](const std::string_view &) const {
    return undefined;
}

inline PCustomValue Value::get_custom() const {
    if (_storage != Storage::custom_type) return nullptr;
    _un.custom->add_ref();
    return PCustomValue(_un.custom);

}

inline Value::Value(std::initializer_list<Value> list) {
    bool is_object = std::all_of(list.begin(),list.end(),[](const Value &v){
        return v.type() == Type::array && v.size() == 2 && v[0].type() == Type::string;
    });
    if (is_object) {
        auto cont = Container<KeyValue>::create(list.size());
        std::transform(list.begin(), list.end(), cont->begin(), [&](const Value &v){
            return KeyValue{v[0].get_string(),v[1]};
        });
        _un.object = sort_object(std::move(cont)).release();
        _storage = Storage::object;
    } else {
        auto cont = Container<Value>::create(list.size());
        std::copy(list.begin(), list.end(), cont->begin());
        _un.array = cont.release();
        _storage = Storage::array;
    }
}



class Value::Iterator: public std::iterator_traits<const Value *> {
public:

    constexpr Iterator():_key_value(false), _v(nullptr) {}
    constexpr Iterator(const Value *v):_key_value(false),_v(v) {}
    constexpr Iterator(const KeyValue *kv):_key_value(true), _kv(kv) {}
    constexpr bool operator==(const Iterator &other) const {
        if (_key_value) {
            return _kv == other._kv;
        } else {
            return _v == other._v;
        }
    }
    constexpr reference operator *() const {return *(_key_value?&_kv->value:_v);}
    constexpr pointer operator ->() const {return _key_value?&_kv->value:_v;}
    constexpr Iterator &operator++() {if (_key_value) ++_kv; else ++_v;return *this;}
    constexpr Iterator operator++(int) {auto cpy = *this; this->operator ++(); return cpy;}
    constexpr Iterator &operator--() {if (_key_value) --_kv; else --_v;return *this;}
    constexpr Iterator operator--(int) {auto cpy = *this; this->operator --(); return cpy;}
    constexpr Iterator &operator+=(difference_type x) {if (_key_value) _kv+=x; else _v+=x;return *this;}
    constexpr Iterator &operator-=(difference_type x) {if (_key_value) _kv-=x; else _v-=x;return *this;}
    constexpr Iterator operator+(difference_type x) const {return _key_value?Iterator(_kv+x):Iterator(_v+x);}
    constexpr Iterator operator-(difference_type x) const {return _key_value?Iterator(_kv-x):Iterator(_v-x);}
    constexpr ptrdiff_t operator-(const Iterator &x) const {return _key_value?_kv-x._kv:_v-x._v;}

protected:
    bool _key_value;
    union { // @suppress("Miss copy constructor or assignment operator")
        const Value *_v;
        const KeyValue *_kv;
    };
};

inline constexpr Value::Iterator Value::begin() const {
    switch (_storage) {
        case Storage::array: return Iterator(_un.array->begin());
        case Storage::object: return Iterator(_un.object->begin());
        default: return Iterator();
    }
}
inline constexpr Value::Iterator Value::end() const {
    switch (_storage) {
        case Storage::array: return Iterator(_un.array->end());
        case Storage::object: return Iterator(_un.object->end());
        default: return Iterator();
    }
}


class Value::KeyAccess {
public:
    constexpr KeyAccess(const Value &owner):_owner(owner) {};
    constexpr KeyValue operator[](unsigned int index) const {
        if (_owner._storage == Storage::object && _owner._un.object->size() > index) {
            return _owner._un.object->data()[index];
        } else {
            return {};
        };
    }
    const KeyValue *begin() const {
        if (_owner._storage == Storage::object) {
            return _owner._un.object->begin();
        } else {
            return {};
        }
    }
    const KeyValue *end() const {
        if (_owner._storage == Storage::object) {
            return _owner._un.object->end();
        } else {
            return {};
        }
    }
    std::size_t size() const {
        if (_owner._storage == Storage::object) {
            return _owner._un.object->size();
        } else {
            return 0;
        }
    }
    constexpr std::span<const KeyValue> get_span() const {
        if (_owner._storage == Storage::object) {
            return std::span<const KeyValue>(_owner._un.object->data(), _owner._un.object->size());
        } else {
            return {};
        }

    }

protected:
    const Value &_owner;
};

inline constexpr Value::KeyAccess Value::keys() const {
    return KeyAccess(*this);
}

inline Value &Value::merge_keys(const Value &changes) {
    PContainer<KeyValue> kv = Container<KeyValue>::create(size()+changes.size());
    auto out = kv->begin();
    auto kv1 = keys();
    auto kv2 = changes.keys();
    auto iter1 = kv1.begin();
    auto iter2 = kv2.begin();
    auto end1 = kv1.end();
    auto end2 = kv2.end();
    while (iter1 != end1 && iter2 != end2) {
        const auto &itm1 = *iter1;
        const auto &itm2 = *iter2;
        int c= itm1.key.compare(itm2.key);
        if (c<0) {
            *out++ = itm1;
            ++iter1;
        } else if (c>0) {
            if (itm2.value.defined()) {
                *out++ = itm2;
            }
            ++iter2;
        } else if (itm2.value.defined()) {
            *out++ = itm2;
            ++iter1;
            ++iter2;
        } else {
            ++iter1;
            ++iter2;
        }
    }
    while (iter1 != end1) {
        *out++ = *iter1;
        ++iter1;
    }
    while (iter2 != end2) {
        if (iter2->value.defined()) {
            *out++ = *iter2;
        }
        ++iter2;
    }
    kv->set_size(out);
    (*this) = Value(std::move(kv));
    return *this;
}

inline Value &Value::set_keys(std::initializer_list<std::pair<std::string_view, Value> > items) {
    PContainer<KeyValue> kv = Container<KeyValue>::create(items.size());
    std::transform(items.begin(), items.end(), kv->begin(), [](const auto &kv){
        return KeyValue{kv.first, kv.second};
    });
    Value tmp(sort_object(std::move(kv)));
    return merge_keys(tmp);
}

inline constexpr bool Value::get(bool defval) const {if (type() == Type::boolean) return get_bool(); else return defval;}
inline constexpr short Value::get(short defval) const  {if (type() == Type::number) return get_short(); else return defval;}
inline constexpr unsigned short Value::get(unsigned short defval) const {if (type() == Type::number) return get_unsigned_short(); else return defval;}
inline constexpr int Value::get(int defval) const {if (type() == Type::number) return get_int(); else return defval;}
inline constexpr unsigned int Value::get(unsigned int defval) const  {if (type() == Type::number) return get_unsigned_int(); else return defval;}
inline constexpr long Value::get(long defval) const {if (type() == Type::number) return get_long(); else return defval;}
inline constexpr unsigned long Value::get(unsigned long defval) const {if (type() == Type::number) return get_unsigned_long(); else return defval;}
inline constexpr long long Value::get(long long defval) const {if (type() == Type::number) return get_long_long(); else return defval;}
inline constexpr unsigned long long Value::get(unsigned long long defval) const {if (type() == Type::number) return get_unsigned_long_long(); else return defval;}
inline constexpr double Value::get(double defval) const {if (type() == Type::number) return get_double(); else return defval;}
inline constexpr float Value::get(float defval) const  {if (type() == Type::number) return get_float(); else return defval;}
inline constexpr std::string_view Value::get(const std::string_view &defval) const {if (type() == Type::string) return get_string(); else return defval;}

class Value::GetHelper {
public:
    constexpr GetHelper(const Value &owner):_owner(owner) {}
    operator bool() const {return _owner.get_bool();}
    operator short() const {return _owner.get_short();}
    operator unsigned short() const {return _owner.get_unsigned_short();}
    operator int() const {return _owner.get_int();}
    operator unsigned int() const {return _owner.get_unsigned_int();}
    operator long() const {return _owner.get_long();}
    operator unsigned long() const {return _owner.get_unsigned_long();}
    operator long long() const {return _owner.get_long();}
    operator unsigned long long() const {return _owner.get_unsigned_long_long();}
    operator std::string() const {return _owner.to_string();}
    operator std::string_view() const {return _owner.get_string();}
    operator double() const {return _owner.get_double();}
    operator float() const {return _owner.get_float();}
protected:
    const Value &_owner;
};

inline Value::GetHelper Value::get() const {return GetHelper(*this);}

template<std::invocable<Value> Fn>
inline Value Value::filter(Fn fn) {
    auto cont = Container<Value>::create(this->size());
    auto iter = cont->begin();
    for (const auto &v: *this) {
        if (fn(v)) *iter++ = v;
    }
    cont->set_size(iter);
    return Value(std::move(cont));
}
template<std::invocable<KeyValue> Fn>
inline Value Value::filter(Fn fn) {
    auto cont = Container<KeyValue>::create(this->size());
    auto iter = cont->begin();
    for (const auto &v: keys()) {
        if (fn(v)) *iter++ = v;
    }
    return Value(std::move(cont));

}

template<InvokableResult<Value, Value> Fn>
inline Value Value::map(Fn fn) {
    auto cont = Container<Value>::create(this->size());
    auto iter = cont->begin();
    for (const auto &v: *this) {
        Value w (( fn(v) ));
        if (w.defined()) *iter++ = w;
    }
    cont->set_size(iter);
    return Value(std::move(cont));
}
template<InvokableResult<Value, KeyValue> Fn>
inline Value Value::map(Fn fn) {
    auto cont = Container<Value>::create(this->size());
    auto iter = cont->begin();
    for (const auto &v: keys()) {
        Value w (( fn(v) ));
        if (w.defined()) *iter++ = w;
    }
    cont->set_size(iter);
    return Value(std::move(cont));
}
template<InvokableResult<KeyValue, KeyValue> Fn>
inline Value Value::map(Fn fn) {
    auto cont = Container<KeyValue>::create(this->size());
    auto iter = cont->begin();
    for (const auto &v: keys()) {
        KeyValue w (( fn(v) ));
        if (w.value.defined()) *iter++ = w;
    }
    return Value(std::move(cont));

}
template<InvokableResult<KeyValue, Value> Fn>
inline Value Value::map(Fn fn) {
    auto cont = Container<KeyValue>::create(this->size());
    auto iter = cont->begin();
    for (const auto &v: *this) {
        KeyValue w (( fn(v) ));
        if (w.value.defined()) *iter++ = w;
    }
    return Value(std::move(cont));

}



template <typename T>
concept PairWithString = requires(T t) {
    {t.first} -> std::convertible_to<std::string_view>;
    t.second;
};




constexpr Container<Value> empty_array = {};

inline const constexpr Container<Value>& Value::get_array() const {
    if (_storage == Storage::array) return *_un.array;
    else return empty_array;
}

constexpr Container<KeyValue> empty_object = {};

inline const constexpr Container<KeyValue>& Value::get_object() const {
    if (_storage == Storage::object) return *_un.object;
    else return empty_object;
}

inline Value& Value::insert(Iterator at, std::initializer_list<Value> data) {
    splice(at,at, data.begin(), data.end());
    return *this;
}

inline Value& Value::insert(Iterator at, Value data) {
    splice(at,at, data.begin(), data.end());
    return *this;
}

inline Value& Value::insert(Iterator at, Iterator beg, Iterator end) {
    splice(at,at, beg, end);
    return *this;
}

inline Value& Value::erase(Iterator from, Iterator to) {
    splice(from, to, begin(), begin());
    return *this;
}

inline Value& Value::append(Value array) {
    const auto &arr1 = get_array();
    const auto &arr2 = array.get_array();
    auto finsz = arr1.size() + arr2.size();
    auto res = Container<Value>::create(finsz);
    auto iter = std::copy(arr1.begin(), arr1.end(), res->begin());
    iter = std::copy(arr2.begin(), arr2.end(), iter);
    res->set_size(iter);
    (*this) = Value(std::move(res));
    return *this;
}

inline Value& Value::append(std::initializer_list<Value> data) {
    const auto &arr1 = get_array();
    auto finsz = arr1.size() + data.size();
    auto res = Container<Value>::create(finsz);
    auto iter = std::copy(arr1.begin(), arr1.end(), res->begin());
    iter = std::copy(data.begin(), data.end(), iter);
    res->set_size(iter);
    (*this) = Value(std::move(res));
    return *this;
}

inline Value Value::slice(Iterator from, Iterator to) {
    return Value(from, to);
}

inline Value Value::splice(Iterator from, Iterator to,
        std::initializer_list<Value> items) {
    return splice(from, to, items.begin(), items.end());
}


template<typename Iter>
inline Value json::Value::splice(Iterator from, Iterator to, Iter new_from, Iter new_to) {
    static_assert(std::is_constructible_v<Value, decltype(*new_from)>);
    auto ersz = std::distance(from, to);
    auto addsz = std::distance(new_from, new_to);
    auto finsz = size() - ersz + addsz;
    auto res = Container<Value>::create(finsz);
    const auto &src = get_array();
    Value erased = slice(from, to);
    auto p = res->begin();
    auto pe = res->end();
    auto rd = src.begin();
    auto re = src.end();
    while (rd != from && p != pe) {
        *p++ = *rd++;
    }
    while (new_from != new_to && p != pe) {
        *p++ = *new_from++;
    }
    rd += ersz;
    while (rd != re && p != pe) {
        *p++ = *rd++;
    }
    res->set_size(p);
    (*this) = Value(std::move(p));
    return erased;
}



template<typename Iter, typename TransformFn>
inline json::Value::Value(Iter from, Iter to, TransformFn fn) {
    using T = std::decay_t<decltype(fn(*from))>;
    if constexpr(std::is_same_v<T, KeyValue>) {
        auto obj = Container<KeyValue>::create(std::distance(from,to));
        std::transform(from, to, obj->begin(), std::forward<TransformFn>(fn));
        (*this) = Value(sort_object(obj));
    } else if constexpr(PairWithString<T>) {
        using Q = std::decay_t<decltype(from->second)>;
        static_assert(std::is_constructible_v<Value, Q>, "Cannot construct Value from the item (pair->object)");
        auto obj = Container<KeyValue>::create(std::distance(from,to));
        std::transform(from, to, obj->begin(), [&](const T &x){
            auto r = fn(x);
            return KeyValue{r.first, r.second};
        });
        (*this) = Value(sort_object(obj));
    } else {
        static_assert(std::is_constructible_v<Value, T>, "Cannot construct Value from the item (array)");
        auto arr = Container<Value>::create(std::distance(from,to));
        std::transform(from, to, arr->begin(), std::forward<TransformFn>(fn));
        (*this) = Value(std::move(arr));
    }
}

template<typename Iter>
inline Value::Value(Iter from, Iter to)
    :Value(from, to, [&](const auto &x) -> decltype(auto){return x;}) {}


inline constexpr bool Value::operator==(const Value &other) const {

    return visit([&](const auto &a){
        using TA = std::decay_t<decltype(a)>;
        if constexpr(std::is_same_v<TA, Undefined>) {
            return false;
        } else {
            return other.visit([&](const auto &b){
               using TB =  std::decay_t<decltype(b)>;
               if constexpr(std::is_same_v<TB, Undefined>) {
                   return false;
               } else if constexpr (std::is_same_v<TA,TB>) {
                   return a==b;
               } else {
                   return false;
               }
            });
        }
    });
}


}
