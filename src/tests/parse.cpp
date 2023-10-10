#include <imtjson/value.h>
#include <imtjson/parser.h>
#include "check.h"

constexpr std::string_view case1 = R"json({
  "string": "Hello,\n World!",
  "number": 42,
  "boolean": true,
  "null_value": null,
  "array": [1, 2, 3],
  "object": {
    "key1": "value1",
    "key2": "value2"
  }
})json";

constexpr std::string_view case2 = R"json([
  "text",
  123,
  true,
  null,
  {
    "key": "value\\value"
  }
])json";

constexpr std::string_view case3 = R"json({
  "unicode_string": "Příklad textu s Unicode znaky: Česká republika",
  "utf8_string": "Toto je řetězec v kódování UTF-8: €¥£"
})json";

constexpr std::string_view case4 = R"json({
  "person": {
    "name": "John Doe",
    "age": 30,
    "address": {
      "street": "123 Main Street",
      "city": "Anytown",
      "zipcode": "12345"
    }
  },
  "fruits": ["apple", "banana", "cherry"]
})json";

constexpr std::string_view case5 = R"json({
  "emoji_string": "Toto je řetězec s několika smajlíky: \ud83d\ude00 \ud83d\ude04 \ud83d\ude0a"
})json";

int main() {

    using namespace json;
    Value jc1 = parse(case1);
    Value jc2 = parse(case2);
    Value jc3 = parse(case3);
    Value jc4 = parse(case4);
    Value jc5 = parse(case5);

    CHECK_EQUAL(jc1["string"].get_string(), "Hello,\n World!");
    CHECK_EQUAL(jc1["number"].get_int(), 42);
    CHECK_EQUAL(jc1["boolean"].get_bool(), true);
    CHECK(jc1["null_value"] == nullptr);
    CHECK_EQUAL(jc1["array"].size(), 3);
    CHECK_EQUAL(jc1["array"][0].get_int(), 1);
    CHECK_EQUAL(jc1["array"][1].get_int(), 2);
    CHECK_EQUAL(jc1["array"][2].get_int(), 3);
    CHECK_EQUAL(jc1["object"]["key1"].get_string(), "value1");
    CHECK_EQUAL(jc1["object"]["key2"].get_string(), "value2");

    CHECK_EQUAL(jc2[0].get_string(),"text");
    CHECK_EQUAL(jc2[1].get_int(),123);
    CHECK(jc2[2].type() == Type::boolean);
    CHECK(jc2[3].type() == Type::null);
    CHECK(jc2[4].type() == Type::object);
    CHECK(jc2[4]["key"].type() == Type::string);
    CHECK_EQUAL(jc2[4]["key"].get_string(),"value\\value");

    CHECK_EQUAL(jc3["unicode_string"].get_string(), "Příklad textu s Unicode znaky: Česká republika");
    CHECK_EQUAL(jc3["utf8_string"].get_string(), "Toto je řetězec v kódování UTF-8: €¥£");
    CHECK_EQUAL(jc4["person"]["name"].get_string(), "John Doe");
    CHECK_EQUAL(jc4["person"]["age"].get_int(), 30);
    CHECK_EQUAL(jc4["person"]["address"]["street"].get_string(), "123 Main Street");
    CHECK_EQUAL(jc4["fruits"].size(), 3);
    CHECK_EQUAL(jc5["emoji_string"].get_string(), "Toto je řetězec s několika smajlíky: 😀 😄 😊");


}
