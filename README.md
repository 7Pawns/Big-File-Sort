# Big File Sort
Big file sort using OS-level APIs.

## Problem
Why not just use merge sort?   
When we sort big files we cannot load the entire text into the RAM, therefore we need to find a way to sort them without actually saving a large amount of information at any given time.   

## Plan
* We first take the big file and break it into smaller segment files.
* We then sort each file independently.
* Afterwards we sort all files into a big file again, and we have a sorted big file.

## Technical Solution
* After we break the file to segment files, we are going to address the problem like the famous "Merge K Sorted Lists" problem.  
* We are going to open every file, read the first line from each one and load it into a minHeap (I used a priority queue). Now the minHeap has all the first lines sorted.   
* Now we are going to pop the first element from the minHeap (the first string int lexicographic order), and write it into a new big file.
* Then we go to the file the element was from, we take the next element (the file pointer advances each time we read a line) and load it into the minHeap.
* We do this until we read through every file and the minHeap is empty.
* We now have a big sorted file.

## Sorting Multiple Big Files
For this, all we have to do is break each file into segment files first, and only then start the the rebuilding.


## TODO:
* Add linux API support.
