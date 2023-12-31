#include "FileSort.h" // includes <Windows.h> and <vector>
#include <exception>
#include <string>
#include <algorithm>
#include <queue>
#include <map>


void FileSort::Sort(const std::vector<std::string>& inFilePaths, const std::string &outFilePath) {
    // Initial value
    startFileNum = 0;
    
    // Delete out file path if exists
    std::wstring temp = std::wstring(outFilePath.begin(), outFilePath.end());
    LPCWSTR lpcwOutFilePath = temp.c_str();

    DeleteFile(lpcwOutFilePath);

    std::vector<HANDLE> hAllTempFiles;

    for (std::string inFilePath : inFilePaths) {
        // Validate each file
        temp = std::wstring(inFilePath.begin(), inFilePath.end());
        LPCWSTR lpcwInFilePath = temp.c_str();
        DWORD dwFileAttrib = GetFileAttributes(lpcwInFilePath);

        if (!(dwFileAttrib != INVALID_FILE_ATTRIBUTES && !(dwFileAttrib & FILE_ATTRIBUTE_DIRECTORY))) {
            throw std::string("File Doesn't Exist");
        }

        // Check if file size exceed maxFileSizeBytes
        HANDLE hBigFile = CreateFile(lpcwInFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hBigFile == INVALID_HANDLE_VALUE) {
            throw std::string("Unable to open file");
        }

        DWORD fileSize = GetFileSize(hBigFile, NULL);
        if (fileSize > MaxFileSizeBytes) {
            throw std::string("File size surpassed the maximum size supplied");
        }
        std::cout << prefixInfo << "File size is: " << fileSize << std::endl;

        // Call division of temp files function
        std::vector<HANDLE> hTempFiles = divide(hBigFile, fileSize, LineSizeBytes, NumberOfLinesPerSegment, startFileNum);

        if (hTempFiles.empty()) {
            throw std::string("hTempFiles is empty");
        }

        // Delete big file
        CloseHandle(hBigFile);

        hAllTempFiles.insert(hAllTempFiles.end(), hTempFiles.begin(), hTempFiles.end());
    }
    
    //DeleteFile(lpcwInFilePath);
    std::cout << prefixInfo << "Finished divide " << std::endl;

    // Create output big file
    temp = std::wstring(outFilePath.begin(), outFilePath.end());
    LPCWSTR wsOutFilePath = temp.c_str();
    HANDLE hOutBigFile = CreateFile(wsOutFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hOutBigFile == INVALID_HANDLE_VALUE) {

        hOutBigFile = CreateFile(wsOutFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hOutBigFile == INVALID_HANDLE_VALUE) {
            throw std::string("Can't open output file handle");
        }

    }

    // Call merging of temp files into big file
    merge(hAllTempFiles, hOutBigFile, LineSizeBytes);

    std::cout << prefixInfo << "Finished sorting into big file" << std::endl;
    

    cleanup();

    std::cout << prefixInfo << "Completed Successfully. " << "New sorted file created: " << outFilePath << std::endl;

}

void FileSort::Sort(const std::string &inFilePath, const std::string &outFilePath) {
    // Initial value
    
    std::wstring temp = std::wstring(outFilePath.begin(), outFilePath.end());
    LPCWSTR lpcwOutFilePath = temp.c_str();
    
    // Deletes out file path if exists
    DeleteFile(lpcwOutFilePath);
    
    // Check if file exists
    temp = std::wstring(inFilePath.begin(), inFilePath.end());
    LPCWSTR lpcwInFilePath = temp.c_str();
    DWORD dwFileAttrib = GetFileAttributes(lpcwInFilePath);

    if (!(dwFileAttrib != INVALID_FILE_ATTRIBUTES && !(dwFileAttrib & FILE_ATTRIBUTE_DIRECTORY))) {
        throw std::string("File Doesn't Exist");
    }

    // Check if file size exceed maxFileSizeBytes
    HANDLE hBigFile = CreateFile(lpcwInFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBigFile == INVALID_HANDLE_VALUE) {
        throw std::string("Unable to open file");
    }

    DWORD fileSize = GetFileSize(hBigFile, NULL);
    if (fileSize > MaxFileSizeBytes) {
        throw std::string("File size surpassed the maximum size supplied");
    }
    std::cout << prefixInfo << "File size is: " << fileSize << std::endl;
    
    // Call division of temp files function
    std::vector<HANDLE> hTempFiles = divide(hBigFile, fileSize, LineSizeBytes, NumberOfLinesPerSegment, startFileNum);

    if (hTempFiles.empty()) {
        std::cout << "Process Failed" << std::endl;
        return;
    }

    // Delete big file
    CloseHandle(hBigFile);
    //DeleteFile(lpcwInFilePath);
    std::cout << prefixInfo << "Finished divide " << std::endl;
    
    // Create output big file
    temp = std::wstring(outFilePath.begin(), outFilePath.end());
    LPCWSTR wsOutFilePath = temp.c_str();
    HANDLE hOutBigFile = CreateFile(wsOutFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hOutBigFile == INVALID_HANDLE_VALUE) {
        
        hOutBigFile = CreateFile(wsOutFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hOutBigFile == INVALID_HANDLE_VALUE) {
            throw std::string("Can't open output file handle");
        }
        
    }
    
    // Call merging of temp files into big file
    merge(hTempFiles, hOutBigFile, LineSizeBytes);

    std::cout << prefixInfo << "Finished sorting into big file" << std::endl;

    // Deleting all segment files
    cleanup();

    std::cout << prefixInfo << "Completed Successfully. " << "New sorted file created: " << outFilePath << std::endl;

}

// Divide Big files into temp files
std::vector<HANDLE> FileSort::divide(const HANDLE &hBigFile, const DWORD &fileSize, const int &LineSizeBytes, const int &NumberOfLinesPerSegment, int &startFileNum) {

    // Will only be created if not already exists
    CreateDirectory(L"segments", NULL);

    // Opening Big File for read and write into segment files
    DWORD nRead = 0;

    std::vector<char> buff(LineSizeBytes);


    // Calculate the amount of segment files needed;
    int segFileCount = floor(((double)fileSize / (double)LineSizeBytes) / (double)NumberOfLinesPerSegment) == ceil(((double)fileSize / (double)LineSizeBytes) / (double)NumberOfLinesPerSegment) ?
        (fileSize / LineSizeBytes) / NumberOfLinesPerSegment : (fileSize / LineSizeBytes) / NumberOfLinesPerSegment + 1;

    // Check that saving a line from each file inside a minHeap won't overflow the memory
    MEMORYSTATUSEX statusEx;
    statusEx.dwLength = sizeof(statusEx);
    GlobalMemoryStatusEx(&statusEx);
    unsigned long long totalMem = statusEx.ullTotalPhys;

    if (segFileCount * LineSizeBytes > totalMem) {
        throw std::string("Program won't have enough memory to sort the big file according to the parameteres supplied. Try to increase numberOfLinesPerSegment.");
    }


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
            if (nRead != 0) {
                words.push_back(std::string(buff.begin(), buff.end()));
            }
            
        }

        //Sort the buffer words
        std::sort(words.begin(), words.end());

        // Create seg file (i.txt)
        std::string segFilePath = "segments/" + std::to_string(i + startFileNum) + ".txt";
        std::cout << prefixInfo << "Writing to: " << segFilePath << std::endl;
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

        hTempFiles.push_back(hSegFile);

    }
    startFileNum += segFileCount;


    return hTempFiles;
}

// Merge vector of temp files into a big file
void FileSort::merge(const std::vector<HANDLE> &hTempFilesVec, const HANDLE &hOutFile, const int &LineSizeBytes) {
    
    // Priority queue works like minheap underneath
    std::multimap<std::string, HANDLE> whichFile;
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

        
        whichFile.insert(std::pair<std::string, HANDLE>(stringBuff, hTemp));
        minHeap.push(stringBuff);
    }

    // Write to big file
    DWORD nWrite = 0;
    HANDLE hPoppedFile = HANDLE();
    while (minHeap.empty() == false)
    {
        if (!WriteFile(hOutFile, minHeap.top().c_str(), LineSizeBytes, &nWrite, NULL)) {
            throw std::string("Problem Writing to big file");
        }
        
        std::string popped = minHeap.top();
        minHeap.pop();

        // Insert new element to minHeap
        // If file ended buff[0] would be nothing so nothing will get pushed to the minheap
        

        if (whichFile.find(popped) != whichFile.end()) {
            hPoppedFile = whichFile.find(popped)->second;
            whichFile.erase(whichFile.find(popped));
        }

        
        if (hPoppedFile == INVALID_HANDLE_VALUE) {
            throw std::string("Unable to open segment file while sorting to big file");
        }

        if (!ReadFile(hPoppedFile, static_cast<void*>(buff.data()), LineSizeBytes, &nRead, NULL)) {
            throw std::string("Unable to read segment file while sorting to big file: " + popped);
            
        }
        if (nRead) {
            std::string stringBuff = std::string(buff.begin(), buff.end());
            whichFile.insert(std::pair<std::string, HANDLE>(stringBuff, hPoppedFile));
            //whichFile[stringBuff] = hPoppedFile;
            minHeap.push(stringBuff);
        }
        else {
            CloseHandle(hPoppedFile);
        }
        
    }

    CloseHandle(hOutFile);
    
    return;
}

// Deletes all segment files and segments directory
void FileSort::cleanup() {
    DWORD segmentsFolderAttr = GetFileAttributesA("segments");
    if (segmentsFolderAttr == INVALID_FILE_ATTRIBUTES) {
        // Directory already cleaned up
        return;
    }
    
    std::cout << prefixInfo << "Deleting segment files..." << std::endl;

    // Delete every seg file
    for (int i = 0; i < startFileNum; ++i) {
        std::string segFilePath = "segments/" + std::to_string(i) + ".txt";
        DeleteFile((std::wstring(segFilePath.begin(), segFilePath.end())).c_str());
    }

    std::cout << prefixInfo << "Deleting segments directory..." << std::endl;

    // Delete segment directory after being emptied
    if (!RemoveDirectory(L"segments")) {
        std::cout << prefixError << "Could not delete segments directory" << std::endl;
    }

}



