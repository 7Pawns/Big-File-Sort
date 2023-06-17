#include <Windows.h>
#include <iostream>
#include <exception>
#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <map>

// Nicer looking messages
const std::string prefix = "[+] ";

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
        std::cout << prefix << "File Doesn't Exist" << std::endl;
        return;
    }

    // Check if file size exceed maxFileSizeBytes
    HANDLE hBigFile = CreateFile(wsInFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBigFile == INVALID_HANDLE_VALUE) {
        std::cout << "Unable to open file" << std::endl;
        std::cout << GetLastError() << std::endl;
        return;
    }
    DWORD fileSize = GetFileSize(hBigFile, NULL);
    if (fileSize > MaxFileSizeBytes) {
        std::cout << prefix << "File size surpassed the maximum size supplied" << std::endl;
    }
    std::cout << prefix << "File size is: " << fileSize << std::endl;
    
    // Call division of temp files function
    std::vector<HANDLE> hTempFiles = divide(hBigFile, fileSize, LineSizeBytes, NumberOfLinesPerSegment);

    if (hTempFiles.empty()) {
        std::cout << "Process Failed" << std::endl;
        return;
    }

    // Delete big file
    CloseHandle(hBigFile);
    //DeleteFile(wsInFilePath);
    std::cout << "finished divide " << std::endl;
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
        if (GetLastError() == 145) {
            std::cout << prefix << "Directory not empty" << std::endl;
        }
        else {
            std::cout << prefix << "Failed to remove segments directory: " << GetLastError() << std::endl;
        }
    }
    
    


}

// Divide Big files into temp files
std::vector<HANDLE> divide(HANDLE hBigFile, DWORD fileSize, int LineSizeBytes, int NumberOfLinesPerSegment) {

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

    // TODO: Calculate file amount according to available memory
    // Calculate the amount of segment files needed;
    int segFileCount = floor(((double)fileSize / (double)LineSizeBytes) / (double)NumberOfLinesPerSegment) == ceil(((double)fileSize / (double)LineSizeBytes) / (double)NumberOfLinesPerSegment) ?
        (fileSize / LineSizeBytes) / NumberOfLinesPerSegment : (fileSize / LineSizeBytes) / NumberOfLinesPerSegment + 1;

    std::cout << prefix << "Amount of segment files created: " << segFileCount << std::endl;

    // TODO: Add file actions overlapping
    // Read and write LineSizeBytes * NumberOfLinesPerSegment into each segment file
    std::vector<HANDLE> hTempFiles;
    DWORD nWrite = 0;
    for (int i = 0; i < segFileCount; ++i) {
        // Instead of reading the entire seg file data at once, we read just LineSizeBytes so we can save it as a word in a vector for easier sorting
        std::vector<std::string> words;
        for (int j = 0; j < NumberOfLinesPerSegment; ++j) {
            if (!ReadFile(hBigFile, static_cast<void*>(buff.data()), LineSizeBytes, &nRead, NULL)) {
                std::cout << prefix << "ReadFile failed: " << GetLastError() << std::endl;
                return std::vector<HANDLE>();
            }

            words.push_back(std::string(buff.begin(), buff.end()));
        }
        std::cout << sizeof(words) << std::endl;

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
        std::string segFilePath = "./segments/" + std::to_string(i) + ".txt";
        HANDLE hSegFile = CreateFile((std::wstring(segFilePath.begin(), segFilePath.end())).c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

        if (!hSegFile) {
            std::cout << prefix << "Segment file creation failed: " << GetLastError() << std::endl;
            return std::vector<HANDLE>();
        }

        // Write the words in sorted order
        for (std::string word : words) {
            if (!WriteFile(hSegFile, word.c_str(), LineSizeBytes, &nWrite, NULL)) {
                std::cout << prefix << "WriteFile failed: " << GetLastError() << std::endl;
                return std::vector<HANDLE>();
            }
        }

        std::cout << prefix << "Written " << nWrite << " bytes to " << segFilePath << std::endl;

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
            std::cout << GetLastError() << std::endl;
        }

        if (!ReadFile(hTemp, static_cast<void*>(buff.data()), LineSizeBytes, &nRead, NULL)) {
            std::cout << GetLastError() << std::endl;
        }
        std::cout << nRead << std::endl;
        std::string stringBuff = std::string(buff.begin(), buff.end());
        std::cout << stringBuff << std::endl;
        whichFile[stringBuff] = hTemp;
        minHeap.push(stringBuff);
    }

    

    // Write to big file
    DWORD nWrite = 0;
    HANDLE hPoppedFile;
    while (minHeap.empty() == false)
    {
        if (!WriteFile(hOutFile, minHeap.top().c_str(), LineSizeBytes, &nWrite, NULL)) {
            std::cout << prefix << "Problem writing to big file: " << GetLastError() << std::endl;
            return;
        }
        
        std::string popped = minHeap.top();
        std::cout << minHeap.top();
        minHeap.pop();

        // Insert new element to minHeap
        // If file ended buff[0] would be nothing so nothing will get pushed to the minheap
        
        hPoppedFile = whichFile[popped];

        if (hPoppedFile == INVALID_HANDLE_VALUE) {
            std::cout << GetLastError << std::endl;
        }

        if (!ReadFile(hPoppedFile, static_cast<void*>(buff.data()), LineSizeBytes, &nRead, NULL)) {
            std::cout << GetLastError() << std::endl;
            
        }
        if (nRead) {
            
            std::string stringBuff = std::string(buff.begin(), buff.end());
            std::cout << "Added the string: " << stringBuff;
            whichFile[stringBuff] = hPoppedFile;
            minHeap.push(stringBuff);
        }
        else {
            std::cout << "File is empty." << std::endl;
            CloseHandle(hPoppedFile);
        }
        
    }
    
    return;
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
    DeleteFile(L"sorted.txt");
    FileSort fs(100, 2, 6);
    fs.Sort("tests/test1.txt", "./sorted.txt");

    return 0;
}


