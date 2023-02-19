#pragma once
#ifndef CATA_SRC_PINYIN_H
#define CATA_SRC_PINYIN_H

#include <iostream>

namespace Pinyin
{

void build_data();

void chinese_to_pinyin( const std::u32string &str, std::vector<std::u32string> &dest );

bool pinyin_match( const std::u32string &str, const std::u32string &qtr );

}
#endif //CATA_SRC_PINYIN_H
