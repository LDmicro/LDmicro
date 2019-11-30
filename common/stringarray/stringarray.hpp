/*******************************************************************************
*  file    : stringarray.hpp
*  created : 12.05.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef STRINGARRAY_HPP
#define STRINGARRAY_HPP


#define __TO_STR(X) #X
#define TO_STR(V) __TO_STR(V)

#define STRINGARRAY_VER_MAJOR 0
#define STRINGARRAY_VER_MINOR 2
#define STRINGARRAY_VER_PATCH 0
#define STRINGARRAY_VER_TWEAK 0

#define STRINGARRAY_VER_STR TO_STR(STRINGARRAY_VER_MAJOR) "." TO_STR(STRINGARRAY_VER_MINOR) "."\
                            TO_STR(STRINGARRAY_VER_PATCH) "." TO_STR(STRINGARRAY_VER_TWEAK)

#include <array>
#include <string>
#include <cstring>
#include <stdexcept>

#include <cstdio>

template <size_t _Size>
class StringArray
{
public:
    typedef StringArray<_Size>           _Myt;
    typedef char                         value_type;
    typedef size_t                       size_type;
    typedef ptrdiff_t                    difference_type;
    typedef value_type*                  pointer;
    typedef const value_type*            const_pointer;
    typedef value_type&                  reference;
    typedef const value_type&            const_reference;
    typedef typename _Myt::pointer       iterator;
    typedef typename _Myt::const_pointer const_iterator;

public:
    void swap(_Myt& _Other)
    { // swap contents with _Other
        std::swap(data_, _Other.data_);
    }

    void swap(_Myt&& _Other)
    { // swap contents with _Other
        data_ = std::move(_Other.data_);
    }

    iterator begin()
    { // return iterator for beginning of mutable sequence
        return &data_[0];
    }

    const_iterator begin() const
    { // return iterator for beginning of nonmutable sequence
        return &data_[0];
    }

    iterator end()
    { // return iterator for end of mutable sequence
        return (begin() + size());
    }

    const_iterator end() const
    { // return iterator for beginning of nonmutable sequence
        return (begin() + size());
    }

    const_iterator cbegin() const
    { // return iterator for beginning of nonmutable sequence
        return (((const _Myt*)this)->begin());
    }

    const_iterator cend() const
    { // return iterator for end of nonmutable sequence
        return (((const _Myt*)this)->end());
    }

    size_type size() const
    { // return length of sequence
        size_type zero_pos = 0;
        for(;zero_pos < _Size; ++zero_pos)
            if(data_[zero_pos] == 0)
                break;
        if (zero_pos == _Size)
            {
                throw std::logic_error("Can't find terminating NULL in StringArray<N>");
            }

        return zero_pos;
    }

    size_type length() const
    {
        return size();
    }

    size_type max_size() const
    { // return maximum possible length of sequence
        return (_Size - 1);
    }

    size_type capacity() const
    {
        return max_size();
    }

    bool empty() const
    { // test if sequence is empty
        return (size() == 0);
    }

    reference at(size_type _Pos)
    { // subscript mutable sequence with checking
        if (size() <= _Pos)
            throw std::range_error("invalid StringArray<N> subscript");
        return data_.at(_Pos);
    }

    const_reference at(size_type _Pos) const
    { // subscript nonmutable sequence with checking
        if (size() <= _Pos)
            throw std::range_error("invalid StringArray<N> subscript");
        return (data_.at(_Pos));
    }

    reference operator[](size_type _Pos)
    { // subscript mutable sequence
#if _ITERATOR_DEBUG_LEVEL > 1
        if (size() <= _Pos)
            {
                throw std::range_error("invalid StringArray<N> subscript");
            }
#endif

        return data_[_Pos];
    }

    const_reference operator[](size_type _Pos) const
    { // subscript nonmutable sequence
#if _ITERATOR_DEBUG_LEVEL > 1
        if (_Size <= _Pos)
            {
                throw std::range_error("invalid StringArray<N> subscript");
            }
#endif /* _ITERATOR_DEBUG_LEVEL */

        return data_[_Pos];
    }

    reference front()
    { // return first element of mutable sequence
        return (data_[0]);
    }

    const_reference front() const
    { // return first element of nonmutable sequence
        return (data_[0]);
    }

    reference back()
    { // return last element of mutable sequence
        return (data_[size() - 1]);
    }

    const_reference back() const
    { // return last element of nonmutable sequence
        return (data_[size() - 1]);
    }

    value_type* data()
    { // return pointer to mutable data array
        return &data_[0];
    }

    const value_type* data() const
    { // return pointer to nonmutable data array
        return &data_[0];
    }

    const value_type* c_str() const
    {
        return data();
    }

    const value_type* str() const
    {
        return data();
    }

public:
    void clear()
    {
        data_[0] = 0;
    }
    void push_back(char c)
    {
        if(size() == max_size())
            throw std::length_error("StringArray<N> already full");
        data_[size()+1] = 0;
        data_[size()+0] = c;
    }

    _Myt& append(size_t n, char c)
    {
        auto _end = size()+n;
        if(_end > max_size())
            throw std::length_error("StringArray<N>: addition not fit.");
        data_[_end] = 0;
        for(size_t i = size(); i < _end; ++i)
            data_[i] = c;
        return *this;
    }

    _Myt& append(const char* data, size_t sz)
    {
        auto _end = size() + sz;
        if(_end > max_size())
            throw std::length_error("StringArray<N>: addition not fit.");
        strncat(data_, data, sz);

        return *this;
    }

    _Myt& append(const char* str_)
    {
        return append(str_, strlen(str_));
    }

    template<size_t K>
    _Myt& append(const StringArray<K>& str_arr)
    {
        return append(str_arr.data(), str_arr.size());
    }

    _Myt& operator+=(char c)
    {
        return append(c,1);
    }

    _Myt& operator+=(const char* data)
    {
        return append(data);
    }

    template<size_t K>
    _Myt& operator+=(const StringArray<K>& str_arr)
    {
        return append(str_arr);
    }

public:
    StringArray(const char* str = nullptr);
    template<size_t K>
    StringArray(const StringArray<K>& str_arr);

private:
    char data_[_Size];
};

template <size_t _Size>
template <size_t K>
StringArray<_Size>::StringArray(const StringArray<K>& str_arr)
{
    auto sz = std::min(str_arr.size(), capacity()-1);
    memcpy(data_, str_arr.data(), sz);
    data_[sz] = 0;
}

template <size_t _Size>
StringArray<_Size>::StringArray(const char* str)
{
    if (str)
        {
            auto sz = std::min(strlen(str), capacity()-1);
            memcpy(data_, str, sz);
            data_[sz] = 0;
        }
    else
        {
            data_[0] = 0;
        }
}

// operators

template <size_t N1, size_t N2>
bool operator==(const StringArray<N1>& lhs, const StringArray<N1>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) == 0;
}

template <size_t N1, size_t N2>
bool operator!=(const StringArray<N1>& lhs, const StringArray<N1>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) != 0;
}

template <size_t N1, size_t N2>
bool operator<(const StringArray<N1>& lhs, const StringArray<N1>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) < 0;
}

template <size_t N1, size_t N2>
bool operator<=(const StringArray<N1>& lhs, const StringArray<N1>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) <= 0;
}

template <size_t N1, size_t N2>
bool operator>(const StringArray<N1>& lhs, const StringArray<N1>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) > 0;
}

template <size_t N1, size_t N2>
bool operator>=(const StringArray<N1>& lhs, const StringArray<N1>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) >= 0;
}

template <size_t N>
bool operator==(const StringArray<N>& lhs, const StringArray<N>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) == 0;
}

template <size_t N>
bool operator!=(const StringArray<N>& lhs, const StringArray<N>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) != 0;
}

template <size_t N>
bool operator<(const StringArray<N>& lhs, const StringArray<N>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) < 0;
}

template <size_t N>
bool operator<=(const StringArray<N>& lhs, const StringArray<N>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) <= 0;
}

template <size_t N>
bool operator>(const StringArray<N>& lhs, const StringArray<N>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) > 0;
}

template <size_t N>
bool operator>=(const StringArray<N>& lhs, const StringArray<N>& rhs)
{
    return strcmp(lhs.c_str(), rhs.c_str()) >= 0;
}

template <size_t N>
bool operator==(const StringArray<N>& lhs, const char* rhs)
{
    return strcmp(lhs.c_str(), rhs) == 0;
}

template <size_t N>
bool operator!=(const StringArray<N>& lhs, const char* rhs)
{
    return strcmp(lhs.c_str(), rhs) != 0;
}

template <size_t N>
bool operator<(const StringArray<N>& lhs, const char* rhs)
{
    return strcmp(lhs.c_str(), rhs) < 0;
}

template <size_t N>
bool operator<=(const StringArray<N>& lhs, const char* rhs)
{
    return strcmp(lhs.c_str(), rhs) <= 0;
}

template <size_t N>
bool operator>(const StringArray<N>& lhs, const char* rhs)
{
    return strcmp(lhs.c_str(), rhs) > 0;
}

template <size_t N>
bool operator>=(const StringArray<N>& lhs, const char* rhs)
{
    return strcmp(lhs.c_str(), rhs) >= 0;
}

#endif // STRINGARRAY_HPP
