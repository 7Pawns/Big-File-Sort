#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <map>
#include <queue>

// Nicer looking messages
const std::string prefixInfo = "[+] ";
const std::string prefixError = "[-] ";

// Used later to manage segment file names
int startFileNum = 0;

class FileSort {
private:
    // Notice that for the minHeap approach if NumberOfTempFiles * LineSizeBytes > availabe memory the process will cause an overflow
    // Because of that we need to make longer files, not more files
    int MaxFileSizeBytes; // Maximum size of the big file
    int NumberOfLinesPerSegment; // Lines per partition
    int LineSizeBytes; // Line syntax: "Something\r\n"
    std::vector<int> divide(const std::string, const int, const int, const int, int&);
    void merge(std::vector<int>, int, int);
public:
    FileSort(int maxFileSizeBytes, int numberOfLinesPerSegment, int lineSizeBytes) {
        MaxFileSizeBytes = maxFileSizeBytes;
        NumberOfLinesPerSegment = numberOfLinesPerSegment;
        LineSizeBytes = lineSizeBytes;
    }
    void Sort(const std::string &inFilePath, const std::string &outFilePath);
    void Sort(const std::vector<std::string> &inFilePaths, const std::string& outFilePath);
    void cleanup();
};

// Implementation for one file
void FileSort::Sort(const std::string &inFilePath, const std::string &outFilePath){
    // Initial value
    startFileNum = 0;

    // Check if file exists
    if (access(inFilePath.c_str(), F_OK) != 0) {
        throw std::string("File doesn't exist");
    } 
    
    // Get file size
    struct stat st;
    stat(inFilePath.c_str(), &st);
    int size = st.st_size;

    std::cout << prefixInfo << "FileSize: " << size << std::endl;

    // Check if file size exceeds limit
    if (size > MaxFileSizeBytes){
        throw std::string("File size exceeds limit");
    }

    std::vector<int> dTempFiles = divide(inFilePath, size, LineSizeBytes, NumberOfLinesPerSegment, startFileNum);

    // Create new big file
    int dOutBigFile = open(outFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0755);

    if (dOutBigFile < 0){
        throw std::string("Couldn't open output file");
    }
    
    merge(dTempFiles, dOutBigFile, LineSizeBytes);

    cleanup();

}

// Take multiple files and sort them into one big file
void FileSort::Sort(const std::vector<std::string> &inFilePaths, const std::string& outFilePath){
    // Initial value
    startFileNum = 0;
    
    std::vector<int> dAllTempFiles;

    for (std::string inFilePath : inFilePaths){
        // Validate each file
        // Check if file exists
        if (access(inFilePath.c_str(), F_OK) != 0) {
            throw std::string("File doesn't exist");
        } 
        
        // Get file size
        struct stat st;
        stat(inFilePath.c_str(), &st);
        int size = st.st_size;

        std::cout << prefixInfo << "FileSize: " << size << std::endl;

        // Check if file size exceeds limit
        if (size > MaxFileSizeBytes){
            throw std::string("File size exceeds limit");
        }

        std::vector<int> dTempFiles = divide(inFilePath, size, LineSizeBytes, NumberOfLinesPerSegment, startFileNum);

        dAllTempFiles.insert(dAllTempFiles.end(), dTempFiles.begin(), dTempFiles.end());
    }

    // Create new big file
    int dOutBigFile = open(outFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0755);

    if (dOutBigFile < 0){
        throw std::string("Couldn't open output file");
    }
    
    merge(dAllTempFiles, dOutBigFile, LineSizeBytes);

    cleanup();
}

// Divide the big file into sorted segment files
std::vector<int> FileSort::divide(const std::string inFilePath, const int fileSize, const int LineSizeBytes, const int NumberOfLinesPerSegment, int &startFileNum){
    std::vector<char> buff(LineSizeBytes);
    int dBigFile = open(inFilePath.c_str(), O_RDONLY);

    if (dBigFile < 0){
        throw std::string("Failed to open big file");
    }

    // Calculate the amount of segment files needed;
    int segFileCount = floor(((double)fileSize / (double)LineSizeBytes) / (double)NumberOfLinesPerSegment) == ceil(((double)fileSize / (double)LineSizeBytes) / (double)NumberOfLinesPerSegment) ?
        (fileSize / LineSizeBytes) / NumberOfLinesPerSegment : (fileSize / LineSizeBytes) / NumberOfLinesPerSegment + 1;

    // Calculate available memory
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    long availableSpace = pages * page_size;

    // Check if enough for execution
    if (availableSpace < segFileCount * LineSizeBytes){
        throw std::string("Program won't have enough memory to sort the big file according to the parameteres supplied. Try to increase numberOfLinesPerSegment.");
    }

    std::cout << prefixInfo << "Amount of segment files created: " << segFileCount << std::endl;

    // Open segments directory
    mkdir("segments", 0755);

    // Start reading from big file into segment files
    size_t nRead;
    std::vector<int> dTempFiles;
    for (int i = 0; i < segFileCount; ++i){
        std::vector<std::string> words;

        for (int j = 0; j < NumberOfLinesPerSegment; ++j){
            nRead = read(dBigFile, static_cast<void*>(buff.data()), LineSizeBytes);

            if (nRead != 0){
                words.push_back(std::string(buff.begin(), buff.end()));
            }

        }

        
        // Sort the buffer words
        std::sort(words.begin(), words.end());

        // Create segment file (i.txt)
        std::string segFilePath = "segments/" + std::to_string(i + startFileNum) + ".txt";
        std::cout << prefixInfo << "Writing to: " << segFilePath << std::endl;
        int dSegFile = open(segFilePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0755);

        if (dSegFile < 0){
            throw std::string("Failed to open segment file");
        }

        // Write the sorted buffer to dSegFile
        size_t nWrite;
        for (std::string word : words){
            nWrite = write(dSegFile, word.c_str(), LineSizeBytes);

            if (!nWrite){
                throw std::string("Failed to write to segment file");
            }

        }

        dTempFiles.push_back(dSegFile);

    }

    startFileNum += segFileCount;
    close(dBigFile);

    return dTempFiles;
}

// Merge the temp files into one sorted big file
void FileSort::merge(std::vector<int> dTempFiles, int dOutFile, int LineSizeBytes){
    // Priority queue works like minheap underneath
    std::multimap<std::string, int> whichFile;
    std::priority_queue<std::string, std::vector<std::string>, std::greater<std::string> > minHeap; 
    std::vector<char> buff(LineSizeBytes);
    size_t nRead;

    for (int dTempFile : dTempFiles){
        // Setting file pointer back to start
        FILE* fp = fdopen(dTempFile, "r");
        rewind(fp);

        // Reading first line from segment file
        nRead = read(dTempFile, static_cast<void*>(buff.data()), LineSizeBytes);
        
        if (!nRead){
            throw std::string("Failed to read from segment file");
        }

        std::string stringBuff = std::string(buff.begin(), buff.end());
        whichFile.insert(std::pair<std::string, int>(stringBuff, dTempFile));
        minHeap.push(stringBuff);
    }

    // Writing to big file until minHeap empties
    int dPoppedFile;
    size_t nWrite;
    while (minHeap.empty() == false){
        nWrite = write(dOutFile, minHeap.top().c_str(), LineSizeBytes);

        if (!nWrite){
            throw std::string("Failed to write to outBigFile");
        }

        std::string popped = minHeap.top();
        minHeap.pop();

        // Insert new element to minHeap from the file the popped word was from
        if (whichFile.find(popped) != whichFile.end()) {
            dPoppedFile = whichFile.find(popped)->second;
            whichFile.erase(whichFile.find(popped));
        }
        
        nRead = read(dPoppedFile, static_cast<void*>(buff.data()), LineSizeBytes);
        if (nRead) {
            std::string stringBuff = std::string(buff.begin(), buff.end());
            whichFile.insert(std::pair<std::string, int>(stringBuff, dPoppedFile));
            minHeap.push(stringBuff);
        }
        else {
            close(dPoppedFile);
        }
    }

    close(dOutFile);

}

void FileSort::cleanup(){
    std::cout << prefixInfo << "Deleting all segment files..." << std::endl;

    // Delete every seg file
    for (int i = 0; i < startFileNum; ++i) {
        std::string segFilePath = "segments/" + std::to_string(i) + ".txt";
        remove(segFilePath.c_str());
    }

    std::cout << prefixInfo << "Deleting segments directory..." << std::endl;

    // Delete segment directory after being emptied
    remove("segments");
}

int main(int argc, char **argv) {
    // int maxFileSizeBytes, int numberOfLinesPerSegment, int lineSizeBytes
    FileSort fs(100, 2, 6);

    try {
        
        std::vector<std::string> inFilePaths = { "tests/test1.txt", "tests/test2.txt" , "tests/test2.txt"};
        fs.Sort(inFilePaths, "./sorted2.txt");  
   
    }
    catch (const std::string &e){
        std::cout << prefixError << e << std::endl;
        fs.cleanup();
        return 1;
    }
    
    return 0;
}