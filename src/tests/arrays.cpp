#include <imtjson/value.h>
#include "check.h"

int main() {

    using namespace json;

    Value arr1 = {1,2,3,4,5,6,7,8,9,10};
    int x = 1;
    for (const auto &v: arr1) {
        CHECK_EQUAL(x, v.get_int());
        ++x;
    }

    Value arr2 = arr1;
    CHECK(arr1 == arr2);
    x = 1;
    for (const auto &v: arr2) {
        CHECK_EQUAL(x, v.get_int());
        ++x;
    }

    Value arr3 = arr1.map([](const Value &x) {
        return std::to_string(x.get_int());
    });

    x = 1;
    for (const auto &v: arr3) {
        CHECK_EQUAL(std::to_string(x), v.get_string());
        ++x;
    }
    Value obj = arr1.map([](const Value &x){
        return KeyValue{std::to_string(x.get_int()), x};
    });
    x = 1;
    for (const auto &v: obj.keys()) {
        auto s = std::to_string(x);
        CHECK_EQUAL(s, v.key);
        CHECK_EQUAL(x, v.value.get_int());
        ++x;
    }

    Value ar41 = Array{1,2,3,"4",5,6,7,8,9,10};
    x = 1;
    for (const auto &v: ar41) {
        CHECK_EQUAL(x, v.get_int());
        ++x;
    }

    Value ar42 = ar41.filter([](const Value &x){
        return (x.get_int() & 1);
    });
    x = 1;
    for (const auto &v: ar42) {
        CHECK_EQUAL(x, v.get_int());
        x+=2;
    }


}
