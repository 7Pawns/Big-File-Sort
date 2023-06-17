#include <Windows.h>
#include <iostream>
#include <exception>
#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <map>

// Nicer looking messages
const std::string prefixInfo = "[+] ";
const std::string prefixError = "[-] ";


std::vector<HANDLE> divide(HANDLE, DWORD, int, int);
void merge(std::vector<HANDLE>, HANDLE, int);

class FileSort {
private:
    // Notice that for the minHeap approach if NumberOfTempFiles * LineSizeBytes > availabe memory the process will cause an overflow
    // Because of that we need to make longer files, not more files
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
        throw std::string("File Doesn't Exist");
    }

    // Check if file size exceed maxFileSizeBytes
    HANDLE hBigFile = CreateFile(wsInFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBigFile == INVALID_HANDLE_VALUE) {
        throw std::string("Unable to open file");
    }

    DWORD fileSize = GetFileSize(hBigFile, NULL);
    if (fileSize > MaxFileSizeBytes) {
        throw std::string("File size surpassed the maximum size supplied");
    }
    std::cout << prefixInfo << "File size is: " << fileSize << std::endl;
    
    // Call division of temp files function
    std::vector<HANDLE> hTempFiles = divide(hBigFile, fileSize, LineSizeBytes, NumberOfLinesPerSegment);

    // TODO: Check if necessary
    if (hTempFiles.empty()) {
        std::cout << "Process Failed" << std::endl;
        return;
    }

    // Delete big file
    CloseHandle(hBigFile);
    //DeleteFile(wsInFilePath);
    std::cout << prefixInfo << "Finished divide " << std::endl;
    
    // Create output big file
    temp = std::wstring(outFilePath.begin(), outFilePath.end());
    LPCWSTR wsOutFilePath = temp.c_str();
    HANDLE hOutBigFile = CreateFile(wsOutFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    
    // Call merging of temp files into big file
    merge(hTempFiles, hOutBigFile, LineSizeBytes);

    int countFile = 0;
    bool noMoreFiles = false;
    std::string segFilePath;
    
    // Delete every seg file
    do {
        segFilePath = "./segments/" + std::to_string(countFile) + ".txt";
        ++countFile;

    } while (DeleteFile((std::wstring(segFilePath.begin(), segFilePath.end())).c_str()));

    // Delete segment directory after being emptied
    if (!RemoveDirectory(L"./segments")) {
        throw std::string("Failed to remove directory");
    }

}

// Divide Big files into temp files
std::vector<HANDLE> divide(HANDLE hBigFile, DWORD fileSize, int LineSizeBytes, int NumberOfLinesPerSegment) {

    // Create Diretory for segment files
    if (!CreateDirectory(L"./segments", NULL)) {
        // Error for already created directory
        throw std::string("Failed to open segments directory");
    }

    // Opening Big File for read and write into segment files
    DWORD nRead = 0;

    std::vector<char> buff(LineSizeBytes);

    // TODO: Calculate file amount according to available memory
    // Calculate the amount of segment files needed;
    int segFileCount = floor(((double)fileSize / (double)LineSizeBytes) / (double)NumberOfLinesPerSegment) == ceil(((double)fileSize / (double)LineSizeBytes) / (double)NumberOfLinesPerSegment) ?
        (fileSize / LineSizeBytes) / NumberOfLinesPerSegment : (fileSize / LineSizeBytes) / NumberOfLinesPerSegment + 1;

    std::cout << prefixInfo << "Amount of segment files created: " << segFileCount << std::endl;

    // Read and write LineSizeBytes * NumberOfLinesPerSegment into each segment file
    std::vector<HANDLE> hTempFiles;
    DWORD nWrite = 0;
    for (int i = 0; i < segFileCount; ++i) {
        
        // Instead of reading the entire seg file data at once, we read just LineSizeBytes so we can save it as a word in a vector for easier sorting
        std::vector<std::string> words;
        for (int j = 0; j < NumberOfLinesPerSegment; ++j) {
            if (!ReadFile(hBigFile, static_cast<void*>(buff.data()), LineSizeBytes, &nRead, NULL)) {
                throw std::string("Failed to read big file");
            }

            words.push_back(std::string(buff.begin(), buff.end()));
        }
        std::cout << sizeof(words) << std::endl;

        std::cout << prefixInfo << "Buffer number " << i << ": " << std::endl;
        std::cout << prefixInfo << "Read " << nRead << " bytes" << std::endl;

        std::cout << prefixInfo << "Words before sort: " << std::endl;
        for (std::string word : words) {
            std::cout << word;
        }
        //Sort the buffer words
        std::sort(words.begin(), words.end());

        std::cout << prefixInfo << "Words after sort: " << std::endl;
        for (std::string word : words) {
            std::cout << word;
        }
        // Create seg file (i.txt)
        std::string segFilePath = "./segments/" + std::to_string(i) + ".txt";
        HANDLE hSegFile = CreateFile((std::wstring(segFilePath.begin(), segFilePath.end())).c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

        if (!hSegFile) {
            throw std::string("Segment file creation failed");
        }

        // Write the words in sorted order
        for (std::string word : words) {
            if (!WriteFile(hSegFile, word.c_str(), LineSizeBytes, &nWrite, NULL)) {
                std::cout << prefixInfo << "WriteFile failed: " << GetLastError() << std::endl;
                throw std::string("Failed writing to segment file");
            }
        }

        std::cout << prefixInfo << "Written " << nWrite << " bytes to " << segFilePath << std::endl;

        hTempFiles.push_back(hSegFile);

    }

    return hTempFiles;
}

// Merge vector of temp files into a big file
void merge(std::vector<HANDLE> hTempFilesVec, HANDLE hOutFile, int LineSizeBytes) {
    
    // Priority queue works like minheap underneath
    std::map<std::string, HANDLE> whichFile;
    std::priority_queue<std::string, std::vector<std::string>, std::greater<std::string> > minHeap; 
    std::vector<char> buff(LineSizeBytes);
    DWORD nRead = 0;

    // Load the minHeap with the fist line from each file 
    for (HANDLE hTemp : hTempFilesVec) {
        // Set pointers back to file's beginning
        if (SetFilePointer(hTemp, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
            throw std::string("Failed to set pointer back to segment file' beginning");
        }

        if (!ReadFile(hTemp, static_cast<void*>(buff.data()), LineSizeBytes, &nRead, NULL)) {
            throw std::string("Failed to read from segment file");
        }

        std::string stringBuff = std::string(buff.begin(), buff.end());
        whichFile[stringBuff] = hTemp;
        minHeap.push(stringBuff);
    }

    // Write to big file
    DWORD nWrite = 0;
    HANDLE hPoppedFile;
    while (minHeap.empty() == false)
    {
        if (!WriteFile(hOutFile, minHeap.top().c_str(), LineSizeBytes, &nWrite, NULL)) {
            throw std::string("Problem Writing to big file");
        }
        
        std::string popped = minHeap.top();
        minHeap.pop();

        // Insert new element to minHeap
        // If file ended buff[0] would be nothing so nothing will get pushed to the minheap
        
        hPoppedFile = whichFile[popped];

        if (hPoppedFile == INVALID_HANDLE_VALUE) {
            throw std::string("Unable to open segment file while sorting to big file");
        }

        if (!ReadFile(hPoppedFile, static_cast<void*>(buff.data()), LineSizeBytes, &nRead, NULL)) {
            throw std::string("Unable to read segment file while sorting to big file");
            
        }
        if (nRead) {
            std::string stringBuff = std::string(buff.begin(), buff.end());
            whichFile[stringBuff] = hPoppedFile;
            minHeap.push(stringBuff);
        }
        else {
            CloseHandle(hPoppedFile);
        }
        
    }
    
    return;
}



int main(int argc, char **argv)
{
    // Deletes if exists
    DeleteFile(L"sorted.txt");
    
    // int maxFileSizeBytes, int numberOfLinesPerSegment, int lineSizeBytes
    FileSort fs(100, 2, 6);
    
    // Handle WINAPI Errors thrown
    try {
        fs.Sort("tests/test1.txt", "./sorted.txt");
    }
    catch (const std::string& e) {
        std::cout << prefixError << e << " WINAPI Error: " << GetLastError() << std::endl;
    }
    
    

    return 0;
}


