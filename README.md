# imtjson20
JSON support for C++ 20, parse / serialize, coroutine friendly, access content, immutable containers,

Header only (src/imtjson/)

The library provides a `json::Value` object that can be used to store a value that can be saved in JSON format. This includes

* numbers (available as int32, int64, double, number in string representation)
* strings
* boolean
* arrays
* associate arrays (objects)
* null

The library also includes the value `undefined`, which represents an uninitialized or nonexistent value (since the value `null` is considered an existing value)

### Construction

* `json::Value()` - construct undefined
* `json::Value(nullptr)` - construct null
* `json::Value({...})` - construct array or object (see below)

### Examples

```
json::Value number(42);
json::Value string("hello world");
json::Value boolean(true);
json::Value array({10,"hello",true});
json::Value object({
    {"a",42},
    {"b","string"}
});`
```

### Constructing array and objects

You can construct an array using initializer list. However you can use the same initializer list to construct an object.

This depends, how the initializon looks like. If result of initialization is array of pairs, where the first of the pair is string, it is considered as object.

```
// object
json::Value object{
    {"a",1},
    {"b",2},
    {"c",3},
    ...
    {"z",99},
};

//not object, it is array of pairs
json::Value not_object{
    {"a",1},
    {"b",2},
    {"c",3},
    ...
    {"z",99},
    42
};

//not object, it is array of pairs
json::Value not_object2{
    {"a",1},
    {"b",2},
    {"c",3},
    ...
    {"z",99},
    {42,"xyz},
};
```
### Force construction of an array

```
json::Value this_is_object {
    {"a",1}
};
//but I need array
json::Value this_is_array = json::Array {
    {"a",1}
};
```

You can also construct na array from std::span<json::Value> , or std::vector<json::Value>, or
generic way `json::Value(Iter from, Iter to)`;

### Inspeciting values

* `.defined()` - test whether value is not undefined
* `.has_value()` - test whether value is defined and is not null
* `.get_<type>()` - retrieve as type, example `get_int` or `get_unsigned_int`, `get_string`, `get_bool`
* `.get()` - retrieve conversion adapter `int v = value.get()`
* `.get(default_value)` - retrieves value if it has expected type, otherwise returns default value
* `.type()` - returns type of value
* `.size()` - if value is container, returns count of items

### Inspecting containers

* `operator[n]` - access n-th value of the container, it works for arrays and objects. Objects
are always ordered by keys. If n is outside of range, returns `undefined`
* `operator[key]`- access value by a key (for object), if not exists, returns `undefined`
* `begin() / end()` - iterators over values

### Inspect object including keys

* `.keys()` - returns adapter, which is able to access keyvalues in case, that value is object
* `.keys()[n]` - returns n-th {key,value} pair
* `.keys().begin()/.keys().begin()` - returns {key,value} iterator

### Modifying containers

All containers are immutable. You cannot modify them unless copy is created.

#### Objects

```
v.set_keys({ {key, value}, {key, value}, ...})
```

This method changes the current object. Replaces all listed keys with new values. You can use the value `undefined` to delete a key. The method creates a new instance of the container containing the changed values and sets this container as the value of the current variable.

The change can only be applied at the current level of nesting. If you need to change a value deeper in the structure, you must propagate the changed containers to higher levels. The example shows how to change the value of "item" in the "sub" key

```
{
   "sub": {
          "item":10
          }
}
```

```
Value v1 = v["sub"];
v1.set_keys({{"item",42}});
v.set_keys({{"sub", v1}});
```

### Arrays

It is recommended to build the array as a vector and then convert the array to json::Value

The splice function can be used to modify the array. It works similarly to javascript, only it uses iterators instead of indexes

```
auto iterv = v.begin()+3; //insert at index 3
Value new_data = {1,2,3};
v.splice(iterv, iterv, new_data.begin(), new_data.end());
```

### Serialization

Serialization uses the Serializer state object. It allows serializing into streams because it generates the serialized result in small chunks that can be easily processed in coroutines

```
json::Serializer ser(value);
auto text = ser.read();
while (!text.empty()) {
    co_await send(text);
    text = ser.read();
}
```

To serialize to a string, use the `json::serialize()` function

```
std::string text = json::serialize(v);
```

### Parsing

Parsing is performed by the Parser. It is also a state object and it also allows to read data in parts, so it is useful in corutines

```
json::Parser prs;
auto text = co_await read_data();
while (prs.write(text)) {
    text = co_await read_data();
}
if (prs.is_error()) throw ; //process error
auto extra_text = prs.get_unprocesssed_data()
json::Value v = prs.get_result();
```

To parse the string, there is a function `json::parse()`

```
json::Value v = json::parse(text);
```
Note that this function may generate a `ParseError` exception

### Binary format

The binary format is a proprietary format supported only by this library, is not standardized, and is intended for communication between programs using this library. 

To create serialize into binary format, use

```
std::string s = json::binarize(value);
```

To parse binary format

```
json::Value value = json::unbinarize(s);
```

There are also classes that can handle binary serializing / parsing

```
json::BinaryParser bp;
json::BinarySerializer bp;
```

### Numbers as text

Numbers can be stored in text form and then saved in this form in the resulting JSON. A more accurate representation of the number can be achieved. Additionally, the Parser always parses the numbers as a string, and as a result, accuracy is not lost during repeated parsing and serialization. At the same time, parsing is faster because the conversion from string to number occurs only when the value is read via the get() interface

To construct number as text, use extended constructor

```
json::Value number("3.1415926535", true);
```
It is necessary to make sure that there is a number in the string, otherwise the result is not valid JSON (the content is not verified)


### Custom values

The Value class offers storage of customized user types. 

```
Value::custom<Type>( arguments...)
```
The type must implement the `AbstractCustomValue` interface

### Constexpr support

Non-container values can be constructed as `constexpr` (including strings)

```
constexpr json::Value c_bool(true);
constexpr json::Value c_number(42);
constexpr json::Value c_text("hello world");
constexpr json::Value c_long text("long long hello world, longer longer");
```

constexpr containers are not supported in C++20

### Internal structure

The whole instance of the Value class takes 16 bytes. The data is stored in union. Most of the types occupy max 8 bytes (pointers, numbers). Strings up to 14 characters are stored directly in the class instance, longer strings are allocated on the heap. Containers (object and array) are implemented with a reference counter and only the reference is copied


**In development**

**Not compatible with old "imtjson"**
