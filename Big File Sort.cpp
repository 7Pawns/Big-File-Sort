#include <Windows.h>
#include <iostream>
#include <exception>

// Nicer looking messages
const std::string prefix = "[+]";
void prefixPrint(std::string text);

class FileSort {
private:
    int MaxFileSizeBytes; // Maximum size of the big file
    int NumberOfLinesPerSegment; // Lines per partition
    int LineSizeBytes; // Line syntax: "Something\r\n"
public:
    FileSort(int maxFileSizeBytes, int numberOfLinesPerSegment, int lineSizeBytes) {
        MaxFileSizeBytes = maxFileSizeBytes;
        NumberOfLinesPerSegment = numberOfLinesPerSegment;
        LineSizeBytes = lineSizeBytes;
    }
    void Sort(const std::string &inFilePath, const std::string &outFilePath);
};

void FileSort::Sort(const std::string &inFilePath, const std::string &outFilePath) {
    // Check if file exists
    std::wstring temp = std::wstring(inFilePath.begin(), inFilePath.end());
    LPCWSTR wsInFilePath = temp.c_str();
    DWORD dwFileAttrib = GetFileAttributes(wsInFilePath);

    if (!(dwFileAttrib != INVALID_FILE_ATTRIBUTES && !(dwFileAttrib & FILE_ATTRIBUTE_DIRECTORY))) {
        std::cout << "File Doesn't Exist" << std::endl;
        return;
    }

    // Check if file size exceed maxFileSizeBytes
    HANDLE hFile = CreateFile(wsInFilePath, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (hFile == NULL) {
        std::cout << GetLastError() << std::endl;
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    std::cout << fileSize << std::endl;

}

int main(int argc, char **argv)
{
    FileSort fs(1, 2, 3);
    fs.Sort("D:/test.txt", "D:/");
}

void prefixPrint(std::string text) {
    std::cout << prefix << " " << text;
}

