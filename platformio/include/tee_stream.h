#ifndef TEE_STREAM_H
#define TEE_STREAM_H

#ifdef NATIVE_TEST
#include "mock_arduino.h"
#else
#include <FS.h>
#include <Stream.h>
#endif

class TeeStream : public Stream {
  public:
    TeeStream(Stream& source, File& dest);
    virtual ~TeeStream();

    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush();
    virtual size_t write(uint8_t); // Not implemented
    virtual String readString();

  private:
    Stream& _source;
    File& _dest;
};

#endif // TEE_STREAM_H
