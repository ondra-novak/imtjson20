#include <imtjson/value.h>
#include "check.h"

int main() {

    using namespace json;

    Value v_undefined;
    Value v_null(nullptr);
    Value v_int32(std::int32_t(1));
    Value v_uint32(std::uint32_t(2));
    Value v_int64(std::int64_t(3));
    Value v_uint64(std::uint64_t(4));
    Value v_double(3.14);
    Value v_shortstr("short str");
    Value v_longstr("long string long string long string");
    Value v_shortnum("1.236483",true);
    Value v_longnum("1154785421889866.236483123",true);
    static constexpr Value c_shortstr("short str");
    static constexpr Value c_longstr("long string long string long string");
    static constexpr Value c_shortnum("1.236483",true);
    static constexpr Value c_longnum("1154785421889866.236483123",true);

    CHECK(!v_undefined.defined());
    CHECK(v_null == nullptr);
    CHECK_EQUAL(v_int32.get_int(),1);
    CHECK_EQUAL(v_uint32.get_unsigned_int(),2);
    CHECK_EQUAL(v_int64.get_long_long(),3);
    CHECK_EQUAL(v_uint64.get_unsigned_long_long(),4);
    CHECK_EQUAL(v_double.get_double(),3.14);
    CHECK_EQUAL(v_shortstr.get_string(),"short str");
    CHECK_EQUAL(v_longstr.get_string(),"long string long string long string");
    CHECK_EQUAL(v_shortnum.get_string(),"1.236483");
    CHECK_EQUAL(v_longnum.get_string(),"1154785421889866.236483123");
    CHECK_EQUAL(c_shortstr.get_string(),"short str");
    CHECK_EQUAL(c_longstr.get_string(),"long string long string long string");
    CHECK_EQUAL(c_shortnum.get_string(),"1.236483");
    CHECK_EQUAL(c_longnum.get_string(),"1154785421889866.236483123");

    CHECK(v_undefined.get_storage() == Storage::undefined);
    CHECK(v_null.get_storage() == Storage::null);
    CHECK(v_int32.get_storage() == Storage::int32);
    CHECK(v_int64.get_storage() == Storage::int64);
    CHECK(v_uint32.get_storage() == Storage::uint32);
    CHECK(v_uint64.get_storage() == Storage::uint64);
    CHECK(v_double.get_storage() == Storage::dnum);
    CHECK(v_shortnum.get_storage() == Storage::short_number_8);
    CHECK(v_longnum.get_storage() == Storage::long_number);
    CHECK(v_shortstr.get_storage() == Storage::short_string_9);
    CHECK(v_longstr.get_storage() == Storage::long_string);
    CHECK(c_shortnum.get_storage() == Storage::short_number_8);
    CHECK(c_longnum.get_storage() == Storage::number_ref);
    CHECK(c_shortstr.get_storage() == Storage::short_string_9);
    CHECK(c_longstr.get_storage() == Storage::string_ref);


}
