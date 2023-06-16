#include <Windows.h>
#include <iostream>
#include <exception>
#include <vector>
#include <string>
#include <algorithm>

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

    std::vector<char> buff(LineSizeBytes);

    // Calculate the amount of segment files needed;
    int segFileCount = floor(((double)fileSize / (double)LineSizeBytes) / (double)NumberOfLinesPerSegment) == ceil(((double)fileSize / (double)LineSizeBytes) / (double)NumberOfLinesPerSegment) ? 
        (fileSize / LineSizeBytes) / NumberOfLinesPerSegment : (fileSize / LineSizeBytes) / NumberOfLinesPerSegment + 1;
   
    std::cout << prefix << "Amount of segment files created: " << segFileCount << std::endl;

    // TODO: Add file actions overlapping
    // Read and write LineSizeBytes * NumberOfLinesPerSegment into each segment file
    DWORD nWrite = 0;
    for (int i = 0; i < segFileCount; ++i) {
        // Instead of reading the entire seg file data at once, we read just LineSizeBytes so we can save it as a word in a vector for easier sorting
        std::vector<std::string> words;
        for (int j = 0; j < NumberOfLinesPerSegment; ++j) {
            if (!ReadFile(hFile, static_cast<void*>(buff.data()), LineSizeBytes, &nRead, NULL)) {
                std::cout << prefix << "ReadFile failed: " << GetLastError() << std::endl;
                return;
            }

            words.push_back(std::string(buff.begin(), buff.end()));
        }

        std::cout << prefix << "Buffer number " << i << ": " << std::endl;
        std::cout << prefix << "Read " << nRead << " bytes" << std::endl;
        
        std::cout << prefix << "Words before sort: " << std::endl;
        for (std::string word : words) {
            std::cout << word;
        }
        //Sort the buffer words
        std::sort(words.begin(), words.end());
        
        std::cout << prefix << "Words after sort: " << std::endl;
        for (std::string word : words) {
            std::cout << word;
        }
        // Create seg file (i.txt)
        std::string segFilePath = "./segments/" + std::to_string(i);
        HANDLE hSegFile = CreateFile((std::wstring(segFilePath.begin(), segFilePath.end())).c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

        if (!hSegFile) {
            std::cout << prefix << "Segment file creation failed: " << GetLastError() << std::endl;
            return;
        }

        // Write the words in sorted order
        if (!WriteFile(hSegFile, &buff, LineSizeBytes * NumberOfLinesPerSegment, &nWrite, NULL)) {
            std::cout << prefix << "WriteFile failed: " << GetLastError() << std::endl;
            return;
        }
        std::cout << prefix << "Written " << nWrite << " bytes to " << segFilePath << ".txt" << std::endl;


        CloseHandle(hSegFile);
    }
    
    // Delete every seg file
    for (int i = 0; i < segFileCount; ++i) {
        std::string segFilePath = "./segments/" + std::to_string(i);
        if (!DeleteFile((std::wstring(segFilePath.begin(), segFilePath.end())).c_str())) {
            std::cout << prefix << "Failed to delete file " << segFilePath << ": " << GetLastError() << std::endl;
        }
    }
    
    // Delete segment directory after being emptied
    if (!RemoveDirectory(L"./segments")) {
        if (GetLastError() == 145) {
            std::cout << prefix << "Directory not empty" << std::endl;
        }
        else {
            std::cout << prefix << "Failed to remove segments directory: " << GetLastError() << std::endl;
        }   
    }

}

int main(int argc, char **argv)
{
    // TODO: Add try and catch
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


