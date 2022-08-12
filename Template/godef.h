#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <vector>

#ifndef GO_CGO_EXPORT_PROLOGUE_H
#define GO_CGO_EXPORT_PROLOGUE_H

#ifndef GO_CGO_GOSTRING_TYPEDEF
typedef struct {
    const char* p;
    ptrdiff_t n;
} _GoString_;
#endif

#endif

#ifndef GO_CGO_PROLOGUE_H
#define GO_CGO_PROLOGUE_H

typedef signed char GoInt8;
typedef unsigned char GoUint8;
typedef short GoInt16;
typedef unsigned short GoUint16;
typedef int GoInt32;
typedef unsigned int GoUint32;
typedef long long GoInt64;
typedef unsigned long long GoUint64;
typedef GoInt64 GoInt;
typedef GoUint64 GoUint;
typedef float GoFloat32;
typedef double GoFloat64;

/*
  static assertion to make sure the file is being used on architecture
  at least with matching size of GoInt.
*/
typedef char _check_for_64_bit_pointer_matching_GoInt[sizeof(void*) == 64 / 8 ? 1 : -1];

#ifndef GO_CGO_GOSTRING_TYPEDEF
typedef _GoString_ GoString;
#endif
typedef void* GoMap;
typedef void* GoChan;
typedef struct {
    void* t;
    void* v;
} GoInterface;
template <typename T>
struct GoSlice {
    T* _data;
    GoInt _len;
    GoInt _cap;
    constexpr GoSlice(const std::vector<T>& vec) {
        _data = vec._data();
        _len = vec.size();
        _cap = vec.capacity();
    }
    constexpr GoSlice(const size_t cap) {
        _data = new T[cap];
        _len = cap;
        _cap = cap;
    }
    constexpr GoSlice(T* data, size_t len, size_t cap) {
        _data = data;
        _len = len;
        _cap = cap;
    }
    constexpr GoSlice() {
        _data = nullptr;
        _len = 0;
        _cap = 0;
    }
    constexpr void push_back(const T& v) {
        resize(_len + 1);
        _data[_len + 1] = v;
        _len++;
    }
    constexpr void resize(size_t new_size) {
        if (new_size <= _cap)
            return;
        T* new_data = new T[new_size];
        memcpy_s(new_data, new_size, _data, _len);
        delete[] _data;
        _data = new_data;
        _cap = new_size;
    }
    constexpr T& operator[](size_t n) {
        return _data[n];
    }
    constexpr size_t length() {
        return _len;
    }
    constexpr T* data() {
        return _data;
    }
    constexpr void shrink_to_fit() {
        if (_data)
            delete[] _data;
    }
};
#endif