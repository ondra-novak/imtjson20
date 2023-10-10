#include <imtjson/value.h>
#include "check.h"

int main() {

    using namespace json;

    Value obj1= {
            {"one",1},
            {"two",2},
            {"three",3},
            {"subobject",{
                    {"one",1},
                    {"two",2},
                    {"three",3}
            }},
            {"subarray",{1,2,nullptr,"text"}}
    };

    CHECK_EQUAL(obj1["one"].get_int(), 1);
    CHECK_EQUAL(obj1["two"].get_int(), 2);
    CHECK_EQUAL(obj1["three"].get_int(), 3);
    CHECK_EQUAL(obj1["subobject"]["one"].get_int(), 1);
    CHECK_EQUAL(obj1["subobject"]["two"].get_int(), 2);
    CHECK_EQUAL(obj1["subobject"]["three"].get_int(), 3);
    CHECK_EQUAL(obj1["subarray"][0].get_int(), 1);
    CHECK_EQUAL(obj1["subarray"][1].get_int(), 2);
    CHECK_EQUAL(obj1["subarray"][2].get_int(), 0);
    CHECK_EQUAL(obj1["subarray"][3].get_int(), 0);
    CHECK(obj1["subarray"][0].defined());
    CHECK(obj1["subarray"][1].defined());
    CHECK(obj1["subarray"][2].defined());
    CHECK(obj1["subarray"][3].defined());
    CHECK(!obj1["subarray"][4].defined());
    CHECK(obj1["subarray"][0].has_value());
    CHECK(obj1["subarray"][1].has_value());
    CHECK(!obj1["subarray"][2].has_value());
    CHECK(obj1["subarray"][0].has_value());

    Value arr = obj1.map([&](const Value &x) {
        return x;
    });

    CHECK_EQUAL(arr[0].get_int(), 1);
    CHECK_EQUAL(arr[4].get_int(), 2);
    CHECK_EQUAL(arr[3].get_int(), 3);
    CHECK_EQUAL(arr[2]["one"].get_int(), 1);
    CHECK_EQUAL(arr[2]["two"].get_int(), 2);
    CHECK_EQUAL(arr[2]["three"].get_int(), 3);
    CHECK_EQUAL(arr[1][0].get_int(), 1);

}
