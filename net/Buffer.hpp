/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <assert.h>
#include <ostream>
#include <vector>

#include <Util.hpp>

// Blocks -> we can share from client -> server ... etc.
// headers / and a 'writeV' etc. =)

/**
 * Encapsulate data we need to write.
 */
class Buffer
{
    std::size_t _size;    /// size of data remaining: buffer.size() - _offset()
    std::size_t _offset;  /// offset into _buffer of data
    std::vector<char> _buffer;

public:
    Buffer() : _size(0), _offset(0)
    {
    }

    typedef std::vector<char>::iterator iterator;
    typedef std::vector<char>::const_iterator const_iterator;

    std::size_t size() const { return _size; }
    bool empty() const { return _size == 0; }

    const char *getBlock() const
    {
        if (_size)
            return &_buffer[_offset];
        return nullptr;
    }

    std::size_t getBlockSize() const
    {
        return _size;
    }

    void eraseFirst(std::size_t len)
    {
        if (len <= 0)
            return;

        assert(_offset + len <= _buffer.size());
        assert(_offset + _size == _buffer.size());

        len = std::min(len, _size); // Avoid accidental damage.

        // avoid regular shuffling down larger chunks of data
        if (_buffer.size() > 16384 && // lots of queued data
            len < _size &&            // not a complete erase
            _offset < 16384 * 64 &&   // do cleanup a Mb at a time or so:
            _size > 512)              // early cleanup if what remains is small.
        {
            _offset += len;
            _size -= len;
            return;
        }

        _buffer.erase(_buffer.begin(), _buffer.begin() + _offset + len);
        _offset = 0;
        _size = _buffer.size();
    }

    void append(const char *data, const int len)
    {
        _buffer.insert(_buffer.end(), data, data + len);
        _size = _buffer.size() - _offset;
    }

    void append(const std::string& s) { append(s.c_str(), s.size()); }

    /// Append a literal string, with compile-time size capturing.
    template <std::size_t N> void append(const char (&s)[N])
    {
        static_assert(N > 1, "Cannot append empty strings.");
        append(s, N - 1); // Minus null termination.
    }

    void dumpHex(std::ostream &os, const char *legend, const char *prefix) const
    {
        if (_size > 0 || _offset > 0)
            os << prefix << "Buffer size: " << _size << " offset: " << _offset << '\n';
        if (_buffer.size() > 0)
            Util::dumpHex(os, _buffer, legend, prefix);
    }

    // various std::vector API compatibility functions

    void clear()
    {
        _buffer.clear();
        _offset = 0;
        _size = 0;
    }

    iterator begin() { return _buffer.begin() + _offset; }

    const_iterator begin() const { return _buffer.begin() + _offset; }

    iterator end() { return _buffer.end(); }

    const_iterator end() const { return _buffer.end(); }

    char operator[](int index) const { return _buffer[_offset + index]; }

    char& operator[](int index) { return _buffer[_offset + index]; }

    const char* data() const { return _buffer.data() + _offset; }

    char* data() { return _buffer.data() + _offset; }

    iterator erase(iterator first, iterator last)
    {
        if (first == begin())
        {
            eraseFirst(last - begin());
            return begin();
        }
        iterator ret = _buffer.erase(first, last);
        _size = _buffer.size() - _offset;
        return ret;
    }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
