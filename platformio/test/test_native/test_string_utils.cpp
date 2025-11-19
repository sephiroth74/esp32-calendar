#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

// Main function that runs all tests with verbose output
int main(int argc, char** argv) {
    doctest::Context context;
    context.setOption("success", true);  // Show all successful tests
    context.setOption("duration", true); // Show test durations
    context.applyCommandLine(argc, argv);
    int res = context.run();
    return res;
}
#include "../mock_arduino.h"
#include "string_utils.h"
#include "../../src/string_utils.cpp"

TEST_SUITE("StringUtils") {
    TEST_CASE("convertToFontEncoding - Latin-1 characters preserved") {
        // Test that UTF-8 encoded Latin-1 characters are converted to single-byte Latin-1
        // The output should contain the actual Latin-1 character (0xE0 for à, etc.)

        // Test lowercase vowels with accents
        String result_a = StringUtils::convertToFontEncoding("à");  // UTF-8: 0xC3 0xA0 -> Latin-1: 0xE0
        CHECK(result_a.length() == 1);
        CHECK((unsigned char)result_a[0] == 0xE0);

        String result_e = StringUtils::convertToFontEncoding("è");  // UTF-8: 0xC3 0xA8 -> Latin-1: 0xE8
        CHECK(result_e.length() == 1);
        CHECK((unsigned char)result_e[0] == 0xE8);

        String result_i = StringUtils::convertToFontEncoding("ì");  // UTF-8: 0xC3 0xAC -> Latin-1: 0xEC
        CHECK(result_i.length() == 1);
        CHECK((unsigned char)result_i[0] == 0xEC);

        String result_o = StringUtils::convertToFontEncoding("ò");  // UTF-8: 0xC3 0xB2 -> Latin-1: 0xF2
        CHECK(result_o.length() == 1);
        CHECK((unsigned char)result_o[0] == 0xF2);

        String result_u = StringUtils::convertToFontEncoding("ù");  // UTF-8: 0xC3 0xB9 -> Latin-1: 0xF9
        CHECK(result_u.length() == 1);
        CHECK((unsigned char)result_u[0] == 0xF9);

        String result_u2 = StringUtils::convertToFontEncoding("ü");  // UTF-8: 0xC3 0xBC -> Latin-1: 0xFC
        CHECK(result_u2.length() == 1);
        CHECK((unsigned char)result_u2[0] == 0xFC);
    }

    TEST_CASE("convertToFontEncoding - Mixed text") {
        // Test mixed text with accents
        String result = StringUtils::convertToFontEncoding("Caffè");
        CHECK(result.length() == 5);
        CHECK(result[0] == 'C');
        CHECK(result[1] == 'a');
        CHECK(result[2] == 'f');
        CHECK(result[3] == 'f');
        CHECK((unsigned char)result[4] == 0xE8);  // è
    }

    TEST_CASE("convertAccents - Legacy function uses new encoding") {
        // convertAccents now uses convertToFontEncoding
        String result = StringUtils::convertAccents("à");
        CHECK(result.length() == 1);
        CHECK((unsigned char)result[0] == 0xE0);
    }

    TEST_CASE("convertAccents - ASCII text unchanged") {
        CHECK(StringUtils::convertAccents("Hello World") == "Hello World");
        CHECK(StringUtils::convertAccents("1234567890") == "1234567890");
        CHECK(StringUtils::convertAccents("!@#$%^&*()") == "!@#$%^&*()");
    }

    TEST_CASE("convertAccents - Empty and special cases") {
        CHECK(StringUtils::convertAccents("") == "");
        CHECK(StringUtils::convertAccents(" ") == " ");
        CHECK(StringUtils::convertAccents("\n\t") == "\n\t");
    }

    TEST_CASE("truncate - Basic truncation") {
        CHECK(StringUtils::truncate("Hello World", 5) == "He...");
        CHECK(StringUtils::truncate("Hello World", 11) == "Hello World");
        CHECK(StringUtils::truncate("Hello World", 15) == "Hello World");
        CHECK(StringUtils::truncate("Test", 4) == "Test");
        CHECK(StringUtils::truncate("Test", 3) == "...");
    }

    TEST_CASE("truncate - Custom suffix") {
        // Note: Using "..." instead of "…" because String.length() counts bytes, not Unicode chars
        CHECK(StringUtils::truncate("Hello World", 9, "...") == "Hello ...");  // 6 chars + 3 = 9
        CHECK(StringUtils::truncate("Hello World", 6, "->") == "Hell->");      // 4 chars + 2 = 6
        CHECK(StringUtils::truncate("Test", 2, "...") == "...");               // suffix only
    }

    TEST_CASE("truncate - Edge cases") {
        CHECK(StringUtils::truncate("", 5) == "");
        CHECK(StringUtils::truncate("Hi", 0) == "...");
        CHECK(StringUtils::truncate("Hi", 1) == "...");
        CHECK(StringUtils::truncate("Hi", 2) == "Hi");
    }

    TEST_CASE("trim - Basic trimming") {
        CHECK(StringUtils::trim("  Hello  ") == "Hello");
        CHECK(StringUtils::trim("\tWorld\n") == "World");
        CHECK(StringUtils::trim("   ") == "");
        CHECK(StringUtils::trim("NoSpaces") == "NoSpaces");
    }

    TEST_CASE("trim - Mixed whitespace") {
        CHECK(StringUtils::trim(" \t\n Hello \n\t ") == "Hello");
        CHECK(StringUtils::trim("\r\nTest\r\n") == "Test");
    }

    TEST_CASE("trim - Edge cases") {
        CHECK(StringUtils::trim("") == "");
        CHECK(StringUtils::trim("a") == "a");
        CHECK(StringUtils::trim(" a ") == "a");
    }

    TEST_CASE("replaceAll - Basic replacement") {
        CHECK(StringUtils::replaceAll("Hello World", "o", "0") == "Hell0 W0rld");
        CHECK(StringUtils::replaceAll("aaabbbccc", "bb", "XX") == "aaaXXbccc");
        CHECK(StringUtils::replaceAll("test test test", "test", "case") == "case case case");
    }

    TEST_CASE("replaceAll - No matches") {
        CHECK(StringUtils::replaceAll("Hello World", "x", "y") == "Hello World");
        CHECK(StringUtils::replaceAll("Test", "Testing", "Tested") == "Test");
    }

    TEST_CASE("replaceAll - Edge cases") {
        CHECK(StringUtils::replaceAll("", "a", "b") == "");
        CHECK(StringUtils::replaceAll("abc", "", "x") == "abc");
        CHECK(StringUtils::replaceAll("abc", "abc", "") == "");
        CHECK(StringUtils::replaceAll("abcabc", "abc", "x") == "xx");
    }

    TEST_CASE("startsWith - Basic checks") {
        CHECK(StringUtils::startsWith("Hello World", "Hello") == true);
        CHECK(StringUtils::startsWith("Hello World", "World") == false);
        CHECK(StringUtils::startsWith("Test", "Te") == true);
        CHECK(StringUtils::startsWith("Test", "test") == false); // Case sensitive
    }

    TEST_CASE("startsWith - Edge cases") {
        CHECK(StringUtils::startsWith("", "") == true);
        CHECK(StringUtils::startsWith("Test", "") == true);
        CHECK(StringUtils::startsWith("", "Test") == false);
        CHECK(StringUtils::startsWith("Hi", "Hello") == false);
        CHECK(StringUtils::startsWith("Test", "Test") == true);
    }

    TEST_CASE("endsWith - Basic checks") {
        CHECK(StringUtils::endsWith("Hello World", "World") == true);
        CHECK(StringUtils::endsWith("Hello World", "Hello") == false);
        CHECK(StringUtils::endsWith("Test", "st") == true);
        CHECK(StringUtils::endsWith("Test", "ST") == false); // Case sensitive
    }

    TEST_CASE("endsWith - Edge cases") {
        CHECK(StringUtils::endsWith("", "") == true);
        CHECK(StringUtils::endsWith("Test", "") == true);
        CHECK(StringUtils::endsWith("", "Test") == false);
        CHECK(StringUtils::endsWith("Hi", "Hello") == false);
        CHECK(StringUtils::endsWith("Test", "Test") == true);
    }

    TEST_CASE("toTitleCase - Basic conversion") {
        CHECK(StringUtils::toTitleCase("hello world") == "Hello World");
        CHECK(StringUtils::toTitleCase("HELLO WORLD") == "Hello World");
        CHECK(StringUtils::toTitleCase("hELLo WoRLD") == "Hello World");
        CHECK(StringUtils::toTitleCase("test") == "Test");
    }

    TEST_CASE("toTitleCase - Multiple spaces") {
        CHECK(StringUtils::toTitleCase("hello  world") == "Hello  World");
        CHECK(StringUtils::toTitleCase("a   b   c") == "A   B   C");
    }

    TEST_CASE("toTitleCase - Special characters") {
        CHECK(StringUtils::toTitleCase("hello-world") == "Hello-World");
        CHECK(StringUtils::toTitleCase("test123test") == "Test123Test");
        CHECK(StringUtils::toTitleCase("one.two.three") == "One.Two.Three");
    }

    TEST_CASE("toTitleCase - Edge cases") {
        CHECK(StringUtils::toTitleCase("") == "");
        CHECK(StringUtils::toTitleCase(" ") == " ");
        CHECK(StringUtils::toTitleCase("a") == "A");
        CHECK(StringUtils::toTitleCase("1234") == "1234");
        CHECK(StringUtils::toTitleCase("!@#$") == "!@#$");
    }

    TEST_CASE("Integration - Multiple operations") {
        // Combine multiple string operations
        String text = "  Università di Milano  ";
        text = StringUtils::trim(text);
        CHECK(text == "Università di Milano");

        // After conversion, accented characters are preserved as Latin-1
        text = StringUtils::convertAccents(text);
        // "Università" = U-n-i-v-e-r-s-i-t-à (10 chars in Latin-1, position 9 is à)
        CHECK(text.length() == 20);  // "Università di Milano" in Latin-1
        CHECK((unsigned char)text[9] == 0xE0);  // à in Latin-1

        // Test truncation with Latin-1 text
        String shortened = StringUtils::truncate(text, 10);
        CHECK(shortened.length() == 10);

        // Another combination with toTitleCase
        String event = "caffè break";
        event = StringUtils::toTitleCase(event);
        CHECK(event == "Caffè Break");

        // After conversion, è becomes Latin-1 0xE8
        event = StringUtils::convertAccents(event);
        CHECK(event.length() == 11);  // "Caffè Break" in Latin-1
        CHECK((unsigned char)event[4] == 0xE8);  // è

        event = StringUtils::replaceAll(event, " ", "_");
        CHECK(event.length() == 11);
        CHECK((unsigned char)event[4] == 0xE8);  // è still preserved
    }
}