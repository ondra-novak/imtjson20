#include <imtjson/value.h>
#include <imtjson/serializer.h>
#include "check.h"


int main() {

    using namespace json;


    Value data = {
            {"m1",42},
            {"abcdefgewwqeq",{
                    1,12.3,43.212,1.2342312e10,0.0,std::numeric_limits<double>::min()
            }},
            {"missing",nullptr},
            {"not here", undefined},
            {"subobject",{
                    {"abc",-123},
                    {"num",Value("123.321000000000001",true)}
            }},
            {"bool1",true},
            {"bool2",false},
            {"inf1", std::numeric_limits<double>::infinity()},
            {"inf2", -std::numeric_limits<double>::infinity()},
            {"nan", std::numeric_limits<double>::signaling_NaN()},
    };

    std::string s = stringify(data);
    CHECK_EQUAL(s, R"JSON({"abcdefgewwqeq":[1,12.3,43.212,1.2342312e+10,0,2.225073858507e-308],"bool1":true,"bool2":false,"inf1":"∞","inf2":"-∞","m1":42,"missing":null,"nan":null,"subobject":{"abc":-123,"num":123.321000000000001}})JSON");


}
