/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2018 ObdDiag.Net. All rights reserved.
 *
 */
 
//
// Lightweight string class, Version: 3.22
//

#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include "lstring.h"

using namespace std;
static int32_t mcount;
    
namespace util {

void string::__init(uint32_t size) noexcept
{
    allocatedLength_ = size > STRING_SIZE ? size : STRING_SIZE;
    data_ = new char[++allocatedLength_]; // Including null terminator
    mcount += allocatedLength_;
}

void string::__move_assign(string& str) noexcept
{
    data_ = str.data_;
    allocatedLength_ = str.allocatedLength_;
    length_ = str.length_;
    str.data_ = nullptr;
    str.allocatedLength_ = str.length_ = 0;
}

string::string(uint32_t size)
{
    __init(size);
    data_[0] = 0;
    length_ = 0;
}

string::string(const char* s)
{
    length_ = strlen(s);
    __init(length_);
    strcpy(data_, s);
}

string::string(const string& str)
{
    length_ = str.length_;
    __init(length_);
    strcpy(data_, str.data_);
}

string::string(uint32_t count, char ch)
{
    length_ = count;
    __init(length_);
    memset(data_, ch, length_);
    data_[length_] = 0;
}

string::string(string&& str) noexcept
{
    __move_assign(str);
}

string::~string()
{
    delete[] data_; 
    mcount -= allocatedLength_;
}

void string::resize(uint32_t count)
{
    if (count < length_) {
        length_ = count;
        data_[length_] = 0;
    }
    else {
        ;
    }
}

void string::resize(uint32_t count, char ch)
{
    if (count < length_) {
        length_ = count;
        data_[length_] = 0;
    }
    else if (count > length_) {
        reserve(count + 1);
        memset(data_ + length_, ch, count - length_);
        length_ = count;
        data_[length_] = 0;
    }
}

void string::reserve(uint32_t size)
{
    if (size > allocatedLength_) {
        char* data = new char[size];
        memcpy(data, data_, length_);
        delete[] data_;
        data_ = data;
        allocatedLength_ = size;
    }
}

string& string::append(const char* s)
{
    int len = strlen(s);
    reserve(length_ + len + 1);
    strcpy(data_ + length_, s);
    length_ += len;
    return *this;
}

string& string::append(const char* s, uint32_t count)
{
    reserve(length_ + count + 1);
    memcpy(data_ + length_, s, count);
    length_ += count;
    data_[length_] = 0;
    return *this;
}

string& string::append(uint32_t count, char ch)
{
    for (uint32_t i = 0; i < count; i++) {
        (*this) += ch;
    }
    return *this;
}

string& string::assign(uint32_t count, char ch)
{
    reserve(count + 1);
    length_ = count;
    memset(data_, ch, count);
    data_[count] = 0;
    return *this;
}

string& string::operator+=(const string& str)
{
    return append(str.data_, str.length_);
}

string& string::operator+=(const char* s)
{
    return append(s);
}

string& string::operator+=(char ch)
{
    reserve(length_ + 2);
    data_[length_] = ch;
    data_[++length_] = 0;
    return *this;
}

uint32_t string::find(const string& str, uint32_t pos) const noexcept
{
    const char* p = strstr(data_ + pos, str.data_);
    return p ? (p - data_) : npos;
}

uint32_t string::find(char ch, uint32_t pos) const noexcept
{
    const char* p = strchr(data_ + pos, ch);
    return p ? (p - data_) : npos;
}

string string::substr(uint32_t pos, uint32_t count) const
{
    uint32_t p = (pos >= length_) ? 0 : pos; // should be an exception
    uint32_t l = (length_ < count) ? (length_ - p) : count;
    string s(l + 1);
    s.append(data_ + p, l);
    return s;
}

string& string::operator=(const string& str)
{
    // Self assignment check
    if (this != &str) {
        reserve(str.length_ + 1);
        memcpy(data_, str.data_, str.length_ + 1); // including null terminator
        length_ = str.length_;
    }
    return *this;
}

string& string::operator=(string&& str) noexcept
{
    // Self assignment check
    if (this != &str) {
        delete[] data_;
        __move_assign(str);
    }
    return *this;
}

string& string::operator=(const char* s)
{
    int len = strlen(s);
    reserve(len + 1);
    strcpy(data_, s);
    length_ = len;
    return *this;
}
void string::clear() noexcept
{
    length_ = 0;
    data_[0] = 0;
}

uint32_t string::copy(char* dest, uint32_t count, uint32_t pos) const
{
    memcpy(dest, data_ + pos, count);
    return count;
}

bool operator==(const string& lhs, const char* rhs)
{
    return (strcmp(lhs.c_str(), rhs) == 0);
}

bool operator!=(const string& lhs, const char* rhs)
{
    return (strcmp(lhs.c_str(), rhs) != 0);
}

bool operator==(const string& lhs, const string& rhs)
{
    return (strcmp(lhs.c_str(), rhs.c_str()) == 0);
}

string operator+(const string& lhs, const string& rhs)
{
    string s(lhs);
    s += rhs;
    return s;
}

string operator+(const char* lhs, const string& rhs)
{
    int len = strlen(lhs) + rhs.length();
    string s(len + 1);
    s += lhs;
    s += rhs;
    return s;
}

string operator+(char ch, const string& rhs)
{
    int len = rhs.length() + 1;
    string s(len + 1);
    s += ch;
    s += rhs;
    return s;
}

string operator+(const string& lhs, const char* rhs)
{
    int len = lhs.length() + strlen(rhs);
    string s(len + 1);
    s += lhs;
    s += rhs;
    return s;
}

string operator+(const string& lhs, char ch)
{
    int len = lhs.length() + 1;
    string s(len + 1);
    s += lhs;
    s += ch;
    return s;
}

} // end util namespace
