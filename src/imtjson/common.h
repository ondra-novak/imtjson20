#pragma once

#include "value.h"

namespace json {

enum class Format: unsigned char {
    text,
    binary
};

namespace BinaryType {
    constexpr unsigned char mask = 0xF8;
    constexpr unsigned char size_mask = 0x7;
    constexpr unsigned char simple = 0x00;

    constexpr unsigned char null = 0x00;
    constexpr unsigned char bool_true = 0x01;
    constexpr unsigned char bool_false = 0x02;
    constexpr unsigned char double_number = 0x03;
    constexpr unsigned char undefined = 0x07;

    constexpr unsigned char p_number = 0x10;
    constexpr unsigned char n_number = 0x18;
    constexpr unsigned char string = 0x20;
    constexpr unsigned char string_number = 0x28;
    constexpr unsigned char array = 0x30;
    constexpr unsigned char object = 0x38;
};



}
