#include <Windows.h>
#include <iostream>
#include <exception>
#include <vector>

// Nicer looking messages
const std::string prefix = "[+] ";

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
        std::cout << prefix << "File Doesn't Exist" << std::endl;
        return;
    }

    // Check if file size exceed maxFileSizeBytes
    HANDLE hFile = CreateFile(wsInFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "Unable to open file" << std::endl;
        std::cout << GetLastError() << std::endl;
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize > MaxFileSizeBytes) {
        std::cout << prefix << "File size surpassed the maximum size supplied" << std::endl;
    }
    std::cout << prefix << "File size is: " << fileSize << std::endl;


    // Create Diretory for segment files
    if (!CreateDirectory(L"./segments", NULL)) {
        // Error for already created directory
        if (GetLastError() == 183) {
            std::cout << prefix << "segments folder already created" << std::endl;
        }
        else {
            std::cout << GetLastError() << std::endl;
        }
    }

    // Opening Big File for read and write into segment files
    DWORD nRead = 0;
    std::vector<char> v(LineSizeBytes*NumberOfLinesPerSegment);
    char *buff = &v[0];

    // Check if there will be N full segments or N full segments and 1 that isn't full;
    double intpart;
    int segFileCount = modf((double)fileSize / (double)LineSizeBytes, &intpart) == 0.0 ? fileSize / LineSizeBytes : fileSize / LineSizeBytes + 1;
    std::cout << prefix << "Amount of segment files created: " << segFileCount << std::endl;

    // Read and write LineSizeBytes * NumberOfLinesPerSegment into each segment file
    for (int i = 0; i < segFileCount; ++i) { }
    if (!ReadFile(hFile, (LPVOID)buff, LineSizeBytes * NumberOfLinesPerSegment, &nRead, NULL)) {
        std::cout << prefix << "ReadFile failed: " << GetLastError() << std::endl;
    }
    std::cout << prefix << "Read " << nRead << " bytes" << std::endl;
    
}

int main(int argc, char **argv)
{
    // TODO: Delete when done
    /*
    if (argc != 3) {
        std::cout << "Crucial parameteres missing. Syntax: ./app <inPath> <outPath>" << std::endl;
        return 1;
    }
    */
     
    // int maxFileSizeBytes, int numberOfLinesPerSegment, int lineSizeBytes
    FileSort fs(100, 2, 6);
    fs.Sort("tests/test1.txt", argv[0]);

    return 0;
}


