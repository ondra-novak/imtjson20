#include <imtjson/value.h>
#include <imtjson/serializer.h>
#include <imtjson/parser.h>
#include "check.h"
#include <iomanip>


int main() {

    using namespace json;


    Value data = {
            {"aaa",{1,2,3}},
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

    std::string s = binarize(data);
   /* for (unsigned char a: s) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(a) << " ";
    }*/

    Value res = unbinarize(s);
    CHECK_EQUAL(stringify(res),stringify(data));
    CHECK_EQUAL(s,binarize(res));





}

template class json::Parser<json::ParserEmptyPreprocesor, json::Format::text>;
template class json::Parser<json::ParserEmptyPreprocesor, json::Format::binary>;
