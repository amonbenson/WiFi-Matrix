#include "FileIO.h"

void FileIO::init(const char* gifDirName) {
    // Set the gif directory name and open the dir
    m_gifDirName = gifDirName;
    m_gifDir = SPIFFS.openDir(m_gifDirName);
    m_gifFileId = 0;

    // Mount SPIFFS
    if (SPIFFS.begin()) {
        DEBUGLN("Mounted SPIFFS")
    } else {
        FATAL("SPIFFS could not be mounted");
    }
}

bool FileIO::onGifFileSeek(unsigned long position) {
    if (!m_gifFile) return false;
    return m_gifFile.seek(position);
}

unsigned long FileIO::onGifFilePosition(void) {
    if (!m_gifFile) return -1;
    return m_gifFile.position();
}

int FileIO::onGifFileRead(void) {
    if (!m_gifFile) return 0;
    return m_gifFile.read();
}

int FileIO::onGifFileReadBlock(void * buffer, int numberOfBytes) {
    if (!m_gifFile) return 0;
    return m_gifFile.read((uint8_t*) buffer, numberOfBytes);
}

void FileIO::nextGifFile() {
    // Close the old file
    if (m_gifFile) m_gifFile.close();

    // Get the next gif file id
    m_gifFileId++;
    if (m_gifFileId >= FileIO::getNumGifFiles()) m_gifFileId = 0;

    // Open the file
    String fileName = getNthGifFileName(m_gifFileId);
    m_gifFile = SPIFFS.open(fileName, "r");
    if (!m_gifFile) {
        WARN("Could not open next Gif file")
        return;
    }
}

void FileIO::prevGifFile() {
    // Close the old file
    if (m_gifFile) m_gifFile.close();

    // Get the next gif file id
    m_gifFileId--;
    if (m_gifFileId < 0) m_gifFileId = FileIO::getNumGifFiles() - 1;

    // Open the file
    String fileName = getNthGifFileName(m_gifFileId);
    m_gifFile = SPIFFS.open(fileName, "r");
    if (!m_gifFile) {
        WARN("Could not open previous Gif file")
        return;
    }
}

String FileIO::getNthGifFileName(int n) {
    // Return empty string if n is invalid
    if (n < 0) return "";

    // Prepare a directory with index counter
    int i = 0;
    Dir gifDir = SPIFFS.openDir(m_gifDirName);

    // Iterate through all files. Return at the correct index
    while (gifDir.next()) {
        if (i == n) return gifDir.fileName();
        i++;
    }

    // No matching file found. Return empty string
    return "";
}

int FileIO::getNumGifFiles() {
    int i = 0;
    Dir gifDir = SPIFFS.openDir(m_gifDirName);

    // Count the number of files and return
    while (gifDir.next()) i++;
    return i;
}