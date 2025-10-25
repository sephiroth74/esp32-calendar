#include <doctest.h>
#include <Arduino.h>
#include "string_utils.h"

TEST_SUITE("StringUtils") {
    TEST_CASE("convertAccents - Italian accented characters") {
        // Test lowercase Italian vowels
        CHECK(StringUtils::convertAccents("è") == "e'");
        CHECK(StringUtils::convertAccents("à") == "a'");
        CHECK(StringUtils::convertAccents("ò") == "o'");
        CHECK(StringUtils::convertAccents("ù") == "u'");
        CHECK(StringUtils::convertAccents("ì") == "i'");

        // Test uppercase Italian vowels
        CHECK(StringUtils::convertAccents("È") == "E'");
        CHECK(StringUtils::convertAccents("À") == "A'");
        CHECK(StringUtils::convertAccents("Ò") == "O'");
        CHECK(StringUtils::convertAccents("Ù") == "U'");
        CHECK(StringUtils::convertAccents("Ì") == "I'");

        // Test mixed text
        CHECK(StringUtils::convertAccents("Caffè") == "Caffe'");
        CHECK(StringUtils::convertAccents("Università") == "Universita'");
        CHECK(StringUtils::convertAccents("Lunedì") == "Lunedi'");
        CHECK(StringUtils::convertAccents("È già mezzogiorno") == "E' gia' mezzogiorno");
    }

    TEST_CASE("convertAccents - Other European characters") {
        // Test French accents
        CHECK(StringUtils::convertAccents("é") == "e'");
        CHECK(StringUtils::convertAccents("É") == "E'");

        // Test text with multiple accents
        CHECK(StringUtils::convertAccents("café") == "cafe'");
        CHECK(StringUtils::convertAccents("naïve") == "na?ve"); // ï not mapped to i'
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
        CHECK(StringUtils::truncate("Hello World", 7, "…") == "Hello …");
        CHECK(StringUtils::truncate("Hello World", 6, "->") == "Hell->");
        CHECK(StringUtils::truncate("Test", 2, "...") == "...");
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

        text = StringUtils::convertAccents(text);
        CHECK(text == "Universita' di Milano");

        text = StringUtils::truncate(text, 15);
        CHECK(text == "Universita' ...");

        // Another combination
        String event = "caffè break";
        event = StringUtils::toTitleCase(event);
        CHECK(event == "Caffè Break");

        event = StringUtils::convertAccents(event);
        CHECK(event == "Caffe' Break");

        event = StringUtils::replaceAll(event, " ", "_");
        CHECK(event == "Caffe'_Break");
    }
}