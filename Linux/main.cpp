#include "FileSort.h"

int main(int argc, char **argv) {
    // int maxFileSizeBytes, int numberOfLinesPerSegment, int lineSizeBytes
    FileSort fs(100, 2, 6);

    try {
        
        std::vector<std::string> inFilePaths = { "tests/test1.txt", "tests/test2.txt" , "tests/test2.txt"};
        fs.Sort(inFilePaths, "sorted.txt");  
   
    }
    catch (const std::string &e){
        std::cout << fs.prefixError << e << std::endl;
        return 1;
    }
    
    return 0;
}