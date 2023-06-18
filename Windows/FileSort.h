#include <iostream>
#include <vector>
#include <Windows.h>

class FileSort {
private:
    // Notice that for the minHeap approach if NumberOfTempFiles * LineSizeBytes > availabe memory the process will cause an overflow
    // Because of that we need to make longer files, not more files
    int MaxFileSizeBytes; // Maximum size of the big file
    int NumberOfLinesPerSegment; // Lines per partition
    int LineSizeBytes; // Line syntax: "Something\r\n"
    int startFileNum = 0;
    
    std::vector<HANDLE> divide(HANDLE, DWORD, int, int, int&);
    void merge(std::vector<HANDLE>, HANDLE, int);
public:
    // Nicer looking messages
    const std::string prefixInfo = "[+] ";
    const std::string prefixError = "[-] ";
    
    FileSort(int maxFileSizeBytes, int numberOfLinesPerSegment, int lineSizeBytes) {
        MaxFileSizeBytes = maxFileSizeBytes;
        NumberOfLinesPerSegment = numberOfLinesPerSegment;
        LineSizeBytes = lineSizeBytes;
    }
    void Sort(const std::string& inFilePath, const std::string& outFilePath);
    void Sort(const std::vector<std::string>& inFilePaths, const std::string& outFilePath);
    void cleanup();

};