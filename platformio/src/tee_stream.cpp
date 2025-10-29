#include "tee_stream.h"

TeeStream::TeeStream(Stream& source, File& dest) : _source(source), _dest(dest) {}

TeeStream::~TeeStream() {
    // The streams are managed externally
}

int TeeStream::available() {
    return _source.available();
}

int TeeStream::read() {
    int c = _source.read();
    if (c != -1) {
        _dest.write((uint8_t)c);
    }
    return c;
}

int TeeStream::peek() {
    return _source.peek();
}

void TeeStream::flush() {
    _source.flush();
    _dest.flush();
}

size_t TeeStream::write(uint8_t) {
    // Not implemented
    return 0;
}

String TeeStream::readString() {
    String result = "";
    while (available()) {
        int c = read();
        if (c == -1) break;
        result += (char)c;
    }
    return result;
}
