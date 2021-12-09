#include <catch.hpp>
#include <fstream>
#include <limits>
#include <gzip/compress.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>

TEST_CASE("successful compress")
{
    std::string data = "hello hello hello hello";

    SECTION("pointer")
    {
        const char* pointer = data.data();
        std::string value = gzip::compress(pointer, data.size());
        REQUIRE(!value.empty());
    }
}

TEST_CASE("fail compress - throws max size limit")
{
    std::string data = "hello hello hello hello";
    const char* pointer = data.data();

    std::uint64_t l = 2000000001;

    CHECK_THROWS_WITH(gzip::compress(pointer, l), Catch::Contains("size may use more memory than intended when decompressing"));
}

#ifdef DEBUG
TEST_CASE("fail compress - pointer, debug throws int overflow")
{
    std::string data = "hello hello hello hello";
    const char* pointer = data.data();
    // numeric_limit useful for integer conversion
    unsigned int i = std::numeric_limits<unsigned int>::max();
    // turn int i into a long, so we can add to it safely without overflow
    unsigned long l = static_cast<unsigned long>(i) + 1;

    CHECK_THROWS_WITH(gzip::compress(pointer, l), Catch::Contains("size arg is too large to fit into unsigned int type"));
}
#endif

TEST_CASE("successful decompress - pointer")
{
    std::string data = "hello hello hello hello";
    const char* pointer = data.data();
    std::string compressed_data = gzip::compress(pointer, data.size());
    const char* compressed_pointer = compressed_data.data();
    std::string value = gzip::decompress(compressed_pointer, compressed_data.size());
    REQUIRE(data == value);
}

#ifdef DEBUG
TEST_CASE("fail decompress - pointer, debug throws int overflow")
{
    std::string data = "hello hello hello hello";
    const char* pointer = data.data();
    std::string compressed_data = gzip::compress(pointer, data.size());
    const char* compressed_pointer = compressed_data.data();

    // numeric_limit useful for integer conversion
    unsigned int i = std::numeric_limits<unsigned int>::max();
    // turn int i into a long, so we can add to it safely without overflow
    unsigned long l = static_cast<unsigned long>(i) + 1;

    CHECK_THROWS_WITH(gzip::decompress(compressed_pointer, l), Catch::Contains("size arg is too large to fit into unsigned int type x2"));
}
#endif

TEST_CASE("invalid decompression")
{
    std::string data("this is a string that should be compressed data");
    const char* pointer = data.data();
    // data is not compressed but we will try to decompress it

    CHECK_THROWS(gzip::decompress(pointer, data.size()));
}

TEST_CASE("round trip compression - gzip")
{
    const std::string data("this is a sentence that will be compressed into something");

    CHECK(!gzip::is_compressed(data.data(), data.size()));

    SECTION("compression level - invalid")
    {
        int level = 99;

        CHECK_THROWS(gzip::compress(data.data(), data.size(), level));
    }

    SECTION("no compression")
    {
        int level = Z_NO_COMPRESSION;
        std::string compressed_data = gzip::compress(data.data(), data.size());
        CHECK(gzip::is_compressed(compressed_data.data(), compressed_data.size()));
        std::string new_data = gzip::decompress(compressed_data.data(), compressed_data.size());
        CHECK(data == new_data);
    }

    SECTION("default compression level")
    {
        int level = Z_DEFAULT_COMPRESSION;
        std::string compressed_data = gzip::compress(data.data(), data.size());
        CHECK(gzip::is_compressed(compressed_data.data(), compressed_data.size()));
        std::string new_data = gzip::decompress(compressed_data.data(), compressed_data.size());
        CHECK(data == new_data);
    }

    SECTION("compression level -- min to max")
    {
        for (int level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; ++level)
        {
            std::string compressed_data = gzip::compress(data.data(), data.size());
            CHECK(gzip::is_compressed(compressed_data.data(), compressed_data.size()));
            std::string new_data = gzip::decompress(compressed_data.data(), compressed_data.size());
            CHECK(data == new_data);
        }
    }
}

TEST_CASE("test decompression size limit")
{
    std::string filename("./test/data/highly_compressed.gz");
    std::ifstream stream(filename, std::ios_base::in | std::ios_base::binary);
    if (!stream.is_open())
    {
        throw std::runtime_error("could not open: '" + filename + "'");
    }
    std::string str_compressed((std::istreambuf_iterator<char>(stream.rdbuf())),
                               std::istreambuf_iterator<char>());
    stream.close();

    std::size_t limit = 20 * 1024 * 1024; // 20 Mb
    // file should be about 500 mb uncompressed
    gzip::Decompressor decomp(limit);
    std::string output;
    CHECK_THROWS(decomp.decompress(output, str_compressed.data(), str_compressed.size()));
    CHECK(output.size() < limit);
}
