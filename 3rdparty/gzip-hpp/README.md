Gzip C++ lib for gzip compression and decompression. Extracted from [mapnik-vector-tile](https://github.com/mapbox/mapnik-vector-tile) for light-weight modularity.

[![Build Status](https://travis-ci.com/mapbox/gzip-hpp.svg?branch=master)](https://travis-ci.com/mapbox/gzip-hpp)
[![hpp-skel badge](https://raw.githubusercontent.com/mapbox/cpp/master/assets/hpp-skel-badge_blue.svg)](https://github.com/mapbox/hpp-skel)

## Usage
```c++
// Include the specific gzip headers your code needs, for example...
#include <gzip/compress.hpp>
#include <gzip/config.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <gzip/version.hpp>

// All function calls must pass in a pointer of an 
// immutable character sequence (aka a string in C) and its size
std::string data = "hello";
const char * pointer = data.data();
std::size_t size = data.size();

// Check if compressed. Can check both gzip and zlib.
bool c = gzip::is_compressed(pointer, size); // false

// Compress returns a std::string
std::string compressed_data = gzip::compress(pointer, size);

// Decompress returns a std::string and decodes both zlib and gzip
const char * compressed_pointer = compressed_data.data();
std::string decompressed_data = gzip::decompress(compressed_pointer, compressed_data.size());

// Or like so
std::string compressed_data = gzip::compress(tile->data(), tile->data.size());

// Or like so
std::string value = gzip::compress(node::Buffer::Data(obj), node::Buffer::Length(obj));

// Or...etc

```
#### Compress
```c++
// Optionally include compression level
std::size_t size; // No default value, but what happens when not passed??
int level = Z_DEFAULT_COMPRESSION; // Z_DEFAULT_COMPRESSION is the default if no arg is passed

std::string compressed_data = gzip::compress(tile->data(), size, level);
```
#### Decompress
```c++
// No args other than the std:string
std::string data = "hello";
std::string compressed_data = gzip::compress(data);
const char * compressed_pointer = compressed_data.data();

std::string decompressed_data = gzip::decompress(compressed_pointer, compressed_data.size());

```

## Test

```shell
# build test binaries
make

# run tests
make test
```

You can make Release test binaries as well
```shell
BUILDTYPE=Release make
BUILDTYPE=Release make test
```

## Versioning

This library is semantically versioned using the /include/gzip/version.cpp file. This defines a number of macros that can be used to check the current major, minor, or patch versions, as well as the full version string.

Here's how you can check for a particular version to use specific API methods
```c++
#if GZIP_VERSION_MAJOR > 2
// use version 2 api
#else
// use older verion apis
#endif
```

Here's how to check the version string
```c++
std::cout << "version: " << GZIP_VERSION_STRING << "/n";
// => version: 0.2.0
```

And lastly, mathematically checking for a specific version:
```c++
#if GZIP_VERSION_CODE > 20001
// use feature provided in v2.0.1
#endif
```
