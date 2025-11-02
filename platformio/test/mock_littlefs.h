#ifndef MOCK_LITTLEFS_H
#define MOCK_LITTLEFS_H

#include <map>
#include <vector>
#include <string>
#include <memory>

// Note: This header must be included AFTER mock_arduino.h defines String and Stream

// Forward declaration
class MockLittleFS;

// Shared file data that persists across File copies
struct FileData {
    std::vector<uint8_t> binaryData;
    std::string path;
    std::string mode;
    size_t position = 0;
    bool isOpen = true;
    MockLittleFS* fs = nullptr;

    ~FileData();
};

// Mock File class with proper persistence - inherits from Stream
class File : public Stream {
public:
    File() : data(nullptr) {}
    File(bool open) : data(open ? std::make_shared<FileData>() : nullptr) {}

    File(const String& content) {
        data = std::make_shared<FileData>();
        data->binaryData.assign(content.c_str(), content.c_str() + content.length());
        data->isOpen = true;
    }

    File(const std::string& path, const std::string& mode, MockLittleFS* fs, const std::vector<uint8_t>& initialData = std::vector<uint8_t>()) {
        data = std::make_shared<FileData>();
        data->path = path;
        data->mode = mode;
        data->fs = fs;
        data->isOpen = true;
        data->position = 0;
        if (!initialData.empty()) {
            data->binaryData = initialData;
        }
    }

    operator bool() const { return data && data->isOpen; }

    int available() override {
        return (data && data->isOpen && data->position < data->binaryData.size()) ?
            (data->binaryData.size() - data->position) : 0;
    }

    String readString() override {
        if (!data || !data->isOpen) return "";
        String result = String(std::string(data->binaryData.begin() + data->position, data->binaryData.end()).c_str());
        data->position = data->binaryData.size();
        return result;
    }

    int read() override {
        if (!data || !data->isOpen || data->position >= data->binaryData.size()) return -1;
        return data->binaryData[data->position++];
    }

    int peek() override {
        if (!data || !data->isOpen || data->position >= data->binaryData.size()) return -1;
        return data->binaryData[data->position];
    }

    size_t write(uint8_t c) {
        if (!data || !data->isOpen) return 0;
        if (data->mode.find('w') != std::string::npos || data->mode.find('a') != std::string::npos) {
            data->binaryData.push_back(c);
            return 1;
        }
        return 0;
    }

    size_t write(const uint8_t* buffer, size_t size) {
        if (!data || !data->isOpen || !buffer) return 0;
        if (data->mode.find('w') != std::string::npos || data->mode.find('a') != std::string::npos) {
            data->binaryData.insert(data->binaryData.end(), buffer, buffer + size);
            return size;
        }
        return 0;
    }

    size_t read(uint8_t* buffer, size_t size) {
        if (!data || !data->isOpen || !buffer || data->position >= data->binaryData.size()) return 0;

        size_t available = data->binaryData.size() - data->position;
        size_t toRead = (size < available) ? size : available;

        std::copy(data->binaryData.begin() + data->position,
                  data->binaryData.begin() + data->position + toRead,
                  buffer);
        data->position += toRead;
        return toRead;
    }

    size_t readBytes(uint8_t* buffer, size_t length) override {
        return read(buffer, length);  // readBytes is just an alias for read
    }

    void close();  // Implemented after MockLittleFS definition

    void flush() override {}

    size_t size() const {
        return data ? data->binaryData.size() : 0;
    }

private:
    std::shared_ptr<FileData> data;
};

// Mock LittleFS filesystem
class MockLittleFS {
public:
    bool begin(bool format = false) {
        if (mountFails) return false;
        mounted = true;
        return true;
    }

    bool exists(const String& path) const {
        return files.find(path.c_str()) != files.end();
    }

    bool exists(const char* path) const {
        return files.find(path) != files.end();
    }

    File open(const String& path, const char* mode = "r") {
        return open(path.c_str(), mode);
    }

    File open(const char* path, const char* mode = "r") {
        std::string pathStr(path);
        std::string modeStr(mode ? mode : "r");

        // Writing mode - create new file
        if (modeStr.find('w') != std::string::npos) {
            return File(pathStr, modeStr, this);
        }

        // Reading mode - return existing file
        auto it = files.find(pathStr);
        if (it != files.end()) {
            return File(pathStr, modeStr, this, it->second);
        }

        return File(false);
    }

    // Called by File::close() to persist data
    void persistFile(const std::string& path, const std::vector<uint8_t>& data) {
        files[path] = data;
    }

    bool remove(const String& path) {
        return remove(path.c_str());
    }

    bool remove(const char* path) {
        auto it = files.find(path);
        if (it != files.end()) {
            files.erase(it);
        }
        return true;
    }

    bool mkdir(const String& path) {
        return true; // Mock - always succeeds
    }

    size_t totalBytes() const {
        return 2375 * 1024; // 2.375MB
    }

    size_t usedBytes() const {
        size_t used = 0;
        for (const auto& pair : files) {
            used += pair.second.size();
        }
        return used;
    }

    // Test helpers
    void addFile(const String& path, const String& content) {
        std::vector<uint8_t> data(content.c_str(), content.c_str() + content.length());
        files[path.c_str()] = data;
    }

    void clear() {
        files.clear();
        mounted = false;
    }

    void setMountFails(bool fails) {
        mountFails = fails;
    }

private:
    std::map<std::string, std::vector<uint8_t>> files;
    bool mounted = false;
    bool mountFails = false;

    friend class File;
};

// FileData destructor - saves file on close
inline FileData::~FileData() {
    if (fs && isOpen && !path.empty() && (mode.find('w') != std::string::npos || mode.find('a') != std::string::npos)) {
        fs->persistFile(path, binaryData);
    }
}

// File::close() implementation
inline void File::close() {
    if (data && data->isOpen) {
        // Persist if writing
        if (data->fs && !data->path.empty() &&
            (data->mode.find('w') != std::string::npos || data->mode.find('a') != std::string::npos)) {
            data->fs->persistFile(data->path, data->binaryData);
        }
        data->isOpen = false;
        data->position = 0;
    }
}

extern MockLittleFS LittleFS;

#endif // MOCK_LITTLEFS_H
