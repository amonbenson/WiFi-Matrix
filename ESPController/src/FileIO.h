#ifndef FILE_IO_H
#define FILE_IO_H

#include <FS.h>
#include "Log.h"


namespace FileIO {
    // Private anonymous namespace
    namespace {
        const char* m_gifDirName;

        Dir m_gifDir;
        File m_gifFile;
        int m_gifFileId;
    }

    void init(const char* gifDirName);

    bool onGifFileSeek(unsigned long position);
    unsigned long onGifFilePosition(void);
    int onGifFileRead(void);
    int onGifFileReadBlock(void * buffer, int numberOfBytes);

    void nextGifFile();
    void prevGifFile();

    int getNumGifFiles();
    String getNthGifFileName(int n);
}

#endif