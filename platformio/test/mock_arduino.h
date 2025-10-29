#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

// Mock Arduino types and functions for native testing
#include <string>
#include <cstring>
#include <cstdint>
#include <ctime>

// Arduino String class mock
class String {
public:
    String() : buffer("") {}
    String(const char* str) : buffer(str ? str : "") {}
    String(const std::string& str) : buffer(str) {}
    String(int val) : buffer(std::to_string(val)) {}
    String(size_t val) : buffer(std::to_string(val)) {}

    const char* c_str() const { return buffer.c_str(); }
    size_t length() const { return buffer.length(); }
    bool isEmpty() const { return buffer.empty(); }

    String substring(size_t start) const {
        if (start >= buffer.length()) return String("");
        return String(buffer.substr(start));
    }

    String substring(size_t start, size_t end) const {
        if (start >= buffer.length()) return String("");
        size_t len = (end > buffer.length()) ? buffer.length() - start : end - start;
        return String(buffer.substr(start, len));
    }

    int indexOf(const String& str) const {
        size_t pos = buffer.find(str.buffer);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }

    int indexOf(const String& str, size_t start) const {
        size_t pos = buffer.find(str.buffer, start);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }

    int indexOf(char c) const {
        size_t pos = buffer.find(c);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }

    void trim() {
        // Trim leading spaces
        size_t start = buffer.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            buffer = "";
            return;
        }
        // Trim trailing spaces
        size_t end = buffer.find_last_not_of(" \t\r\n");
        buffer = buffer.substr(start, end - start + 1);
    }

    void replace(const String& from, const String& to) {
        size_t pos = 0;
        while ((pos = buffer.find(from.buffer, pos)) != std::string::npos) {
            buffer.replace(pos, from.buffer.length(), to.buffer);
            pos += to.buffer.length();
        }
    }

    String& operator=(const String& other) {
        buffer = other.buffer;
        return *this;
    }

    String& operator+=(const String& other) {
        buffer += other.buffer;
        return *this;
    }

    String& operator+=(char c) {
        buffer += c;
        return *this;
    }

    bool operator==(const String& other) const {
        return buffer == other.buffer;
    }

    bool operator!=(const String& other) const {
        return buffer != other.buffer;
    }

    bool operator<(const String& other) const {
        return buffer < other.buffer;
    }

    char operator[](size_t index) const {
        if (index >= buffer.length()) return '\0';
        return (unsigned char)buffer[index];
    }

    char& operator[](size_t index) {
        static char dummy = '\0';
        if (index >= buffer.length()) return dummy;
        return buffer[index];
    }

    char charAt(size_t index) const {
        if (index >= buffer.length()) return '\0';
        return (unsigned char)buffer[index];
    }

    void setCharAt(size_t index, char c) {
        if (index < buffer.length()) {
            buffer[index] = c;
        }
    }

    void remove(size_t index) {
        if (index < buffer.length()) {
            buffer.erase(index);
        }
    }

    void remove(size_t index, size_t count) {
        if (index < buffer.length()) {
            buffer.erase(index, count);
        }
    }

    bool startsWith(const String& prefix) const {
        return buffer.find(prefix.buffer) == 0;
    }

    bool startsWith(const char* prefix) const {
        if (!prefix) return false;
        return buffer.find(prefix) == 0;
    }

    friend String operator+(const String& a, const String& b) {
        return String(a.buffer + b.buffer);
    }

    int toInt() const {
        try {
            return std::stoi(buffer);
        } catch (...) {
            return 0;
        }
    }

private:
    std::string buffer;
};

// Mock Serial for debugging
class MockSerial {
public:
    void begin(int baudrate) {}
    void println(const String& str) {}
    void println(const char* str) {}
    void println(int val) {}
    void print(const String& str) {}
    void print(const char* str) {}

    template<typename... Args>
    void printf(const char* format, Args... args) {
        // Mock implementation - just ignore for testing
    }
};

extern MockSerial Serial;

// Mock Stream class
class Stream {
public:
    virtual ~Stream() {}
    virtual int available() = 0;
    virtual String readString() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual void flush() {}
};

// Mock StringStream for testing
class StringStream : public Stream {
public:
    StringStream(const String& data) : content(data), position(0) {}

    int available() override {
        return content.length() - position;
    }

    String readString() override {
        if (position >= content.length()) {
            return "";
        }
        String result = content.substring(position);
        position = content.length();
        return result;
    }

    int read() override {
        if (position >= content.length()) {
            return -1;
        }
        return content.charAt(position++);
    }

    int peek() override {
        if (position >= content.length()) {
            return -1;
        }
        return content.charAt(position);
    }

    void reset() {
        position = 0;
    }

private:
    String content;
    size_t position;
};

// Mock File class
class File {
public:
    File() : isOpen(false), position(0) {}
    File(bool open) : isOpen(open), position(0) {}
    File(const String& data) : isOpen(true), content(data), position(0) {}

    operator bool() const { return isOpen; }
    bool available() const { return isOpen && position < content.length(); }
    String readString() {
        if (!isOpen) return "";
        String result = content.substring(position);
        position = content.length();
        return result;
    }
    void close() { isOpen = false; position = 0; }

    size_t write(uint8_t c) {
        if (!isOpen) return 0;
        content += String((char)c);
        return 1;
    }

    void flush() {
        // Mock implementation - do nothing
    }

    // For testing purposes
    void setContent(const String& data) { content = data; position = 0; }

private:
    bool isOpen;
    String content;
    size_t position;
};

// Mock LittleFS
#include <map>

class MockLittleFS {
public:
    bool begin(bool format = false) {
        if (mountFails) {
            return false;
        }
        mounted = true;
        return true;
    }

    bool exists(const String& path) {
        auto it = files.find(path.c_str());
        return it != files.end();
    }

    File open(const String& path, const char* mode) {
        auto it = files.find(path.c_str());
        if (it != files.end()) {
            return File(it->second);
        }
        return File(false);
    }

    // For testing - add a file to the mock filesystem
    void addFile(const String& path, const String& content) {
        files[path.c_str()] = content;
    }

    // For testing - clear all files
    void clear() {
        files.clear();
        mounted = false;
    }

    // For testing - simulate mount failure
    void setMountFails(bool fails) {
        mountFails = fails;
    }

    bool mkdir(const String& path) {
        // Mock implementation - just return success
        return true;
    }

private:
    std::map<std::string, String> files;
    bool mounted = false;
    bool mountFails = false;

    friend class MockLittleFS;
};

extern MockLittleFS LittleFS;

// Mock HTTPClient
class HTTPClient {
public:
    bool begin(const String& url) { return true; }
    int GET() { return 200; }
    String getString() { return "BEGIN:VCALENDAR..."; }
    void end() {}
};

// Mock WiFiClient
class WiFiClient {
public:
    // Add any necessary member functions for WiFiClient
};


// Mock time_t if not available
#ifndef _TIME_T
#define _TIME_T
typedef long time_t;
#endif

// Mock Arduino timing functions
inline unsigned long millis() {
    // Return incrementing timestamp to avoid infinite loops
    static unsigned long mockTime = 0;
    return mockTime++;
}

inline void delay(unsigned long ms) {
    // Mock implementation - advance mock time
    static unsigned long mockTime = 0;
    mockTime += ms;
}

#endif // MOCK_ARDUINO_H
