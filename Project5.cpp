//============================================================================
// Name        : Project5.cpp
// Author      : Alexander Lewis
//============================================================================

// Simulates a virtual memory system by processing input logical addresses and translating them using a TLB and page table
// into their physical counterpart in order to retrieve the proper data from a simulated physical memory.

// NOTE: In order to run tests, uncomment the test start methods (if applicable) and test result methods from the beginning
// and end of the main method (to find easily ctrl+f and look for TEST using case sensitive). Also set the input file to
// the appropriate .txt file (they are written next to the start and result methods).

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
using namespace std;

// System parameters
const int PAGETABLE_SIZE = 256; // Size of the page table
const int PAGE_SIZE = 256; // Size of the pages
const int FRAME_SIZE = 256; // Size of the frames
const int FRAME_ENTRIES = 256; // Number of physical frames.
const int TLB_ENTRIES = 16; // maximum number of TLB entries.
const int PAGE_ENTRIES = 256; // Number of logical pages
const int PHYSMEM_SIZE = FRAME_ENTRIES * FRAME_SIZE; // The size of the physical memory
const int ADDRESS_BYTES = 2; // The number of bytes per address in the BACKING_STORE

const char versionNum[] = "1.0"; // The version number
char userInput[4]; // Stores the user input
int repAlgo = 0; // The replacement algorithm used for the TLB (1 = FIFO, 2 = LRU)
int displayAdd = 0; // Whether or not to display the physical addresses (0 for no, 1 for yes)
int pageFaults = 0; // Increment when there is a page fault
int TLBhit = 0; // Increment when there is a TLB hit.

// Files
FILE *BACKING_STORE;
ifstream instream;
char inputFile[] = "InputFile.txt"; // File where the input is stored
//char physMemFile[] = "testInput.txt"; // Where the physical memory initialization data is stored.
char outputFile[] = "vm_sim_output.txt"; // File where the output is stored

// Data storage
unsigned short *inputStorage = new unsigned short[0]; // Pointer to the array where the inputs are stored
int inputStorage_size = 0; // The size of the inputStorage array
// The TLB, stores 16-bit integers with the first 8-bits equal to the page number and
// the last 8 the frame number.
unsigned short TLB[TLB_ENTRIES];
int TLB_index; // Keeps track of the number of elements in the TLB
// The page table, stores 16-bit integers with the first 8-bits equal to the page number and
// the last 8 the frame number.
unsigned short pageTable[PAGETABLE_SIZE]; // The page table
int pageTable_size = 0;
unsigned char physMem[PHYSMEM_SIZE]; // The physical memory

// functions
void readAndCopy(); // Implemented (look into duplicate bug)
unsigned char getMSB(unsigned short logicalAddress); // Implemented
unsigned char getLSB(unsigned short logicalAddress); // Implemented
unsigned short createPhysAdd(unsigned short logicalAddress, unsigned char frameNum);
unsigned short checkTLB(unsigned short logicalAddress); // Implemented
void updateTLB(unsigned short logicalAddress, unsigned short frameNum); // Implemented (test the rep algorithms)
unsigned short checkPageTable(unsigned short logicalAddress); // Implemented
void updatePageTable(unsigned short logicalAddress, unsigned short frameNum);
void createPageTable(); // Unused
unsigned char checkPhysMem(unsigned short physicalAddress); // Implemented
void createPhysMem(); // Unused
void createPhysMem(char fileName[]); // Unused
void loadFromBacking(unsigned short frameNum); // Implemented
void test1(); // implemented
void test1Results(); //implemented
void test2Results();

int main(void) {

	//createPhysMem(); // Create the physical memory
	//createPageTable(); // Create the page table
	readAndCopy(); // Read in the inputs.

	// TESTS: test starters, make sure to comment out on final submission
	//test1(); // test1.txt

	// Standard greeting
	cout << "Welcome to Alexander Lewis's VM Simulator Version " << versionNum << "\n\n";

	// System info
	cout << "Number of logical pages: " << PAGE_ENTRIES << "\n";
	cout << "Page size: " << PAGE_SIZE << " bytes\n";
	cout << "Page table size: " << PAGETABLE_SIZE << "\n";
	cout << "TLB size: " << TLB_ENTRIES << "\n";
	cout << "Number of physical frames: " << FRAME_ENTRIES << "\n";
	cout << "Physical memory size: " << PHYSMEM_SIZE << " bytes\n\n";

	// Prompt to show physical addresses
	int check = 0; // Will be set to 0 until the user passes a valid input
	while (check == 0) {
		cout << "Display Physical Addresses? [y or n] ";
		cin >> userInput;

		// Change the display option based on the input
		if (userInput[0] == 'y' || userInput[0] == 'Y') {
			displayAdd = 1;
			check++;
		}
		else if (userInput[0] == 'n' || userInput[0] == 'N') {
			displayAdd = 0;
			check++;
		}
	}

	// Prompt to decide TLB replacement algorithm
	check = 0; // Will be set to 0 until the user passes a valid input
	while (check == 0) {
		cout << "Choose TLB Replacement Strategy [1: FIFO, 2: LRU] ";
		cin >> userInput;

		// Change the display option based on the input
		if (userInput[0] == '1') {
			repAlgo = 1;
			check++;
		}
		else if (userInput[0] == '2') {
			repAlgo = 2;
			check++;
		}
	}

	// Start writing in the output file
	ofstream outStream;
	outStream.open(outputFile);

	// Go through each of the inputs
	unsigned short logicalAddress; // The logical address being processed
	unsigned short physicalAddress = 0; // The physical address associated to the logical
	unsigned short value = 0; // The byte value at the physical address
	unsigned short outcome; // Used for the individual function returns
	for (int i = 0; i < inputStorage_size; i++) {
		logicalAddress = inputStorage[i]; // Set the logical address

		// look in the TLB
		outcome = checkTLB(logicalAddress);

		// on a TLB miss
		if (outcome == 65280) {

			// Let's try the page table
			outcome = checkPageTable(logicalAddress);

			// On a page fault
			if (outcome == 65280) {

				// Update the physical memory then and update the page table and TLB
				outcome = getMSB(logicalAddress);
				// Update the physical memory at the page number indicated by the logical address
				loadFromBacking(outcome);
				// Update the page table and TLB
				updatePageTable(logicalAddress, outcome);
				updateTLB(logicalAddress, outcome);
				// make the physical address
				physicalAddress = createPhysAdd(logicalAddress, outcome);

			}
			// On a page hit
			else {
				// We found it so make the physical address
				physicalAddress = createPhysAdd(logicalAddress, outcome);
				// Don't forget to update the TLB
				updateTLB(logicalAddress, outcome);
			}

		}

		// on a TLB hit
		else {
			physicalAddress = createPhysAdd(logicalAddress, outcome); // We found it so make the physical address
		}

		// By this point, we should have the physical address, so lets get the value.
		value = checkPhysMem(physicalAddress);

		// Print the results (if so specified)
		if (displayAdd == 1) {
			cout << "Virtual address: " << logicalAddress << "; Physical address: ";
			cout << physicalAddress << "; Value: " << value << "\n";
		}

		// Write the results to the output file
		outStream << "Virtual address: " << logicalAddress << "; Physical address: ";
		outStream << physicalAddress << "; Value: " << value << "\r\n";

	}

	// Calculate the page fault and TLB hit rate
	//printf("pageFaults: %u\n", pageFaults);
	double faultRate = (double) pageFaults / (double) inputStorage_size;
	faultRate *= 100;
	double hitRate = (double) TLBhit / (double) inputStorage_size;
	hitRate *= 100;

	// Display the results
	cout << "\nPage fault rate: " << faultRate << "\n";
	cout << "TLB hit rate: " << hitRate << "\n";

	// Write the results to the output file.
	outStream << "\r\n" << "Page fault rate: " << faultRate << "\r\n";
	outStream << "TLB hit rate: " << hitRate << "\r\n";

	// Stop writing to the output file
	outStream.close();

	// Tell the user where to find the results
	cout << "\nCheck the results in the outputfile: " << outputFile;

	// TESTS: outputs the results of tests, make sure to comment out on final submission.
	//test1Results(); //test1.txt
	//test2Results(); //test2.txt

	return EXIT_SUCCESS;
}

// Reads the 32-bit inputs from the input file and stores them in the inputStorage array.
// NOTE: Stores the 32-bit in shorts so it is stored as 16-bit.
void readAndCopy() {

	// Read the file
	instream.open(inputFile);

    int f = 0;
    unsigned short newStorageArray[inputStorage_size];
    unsigned short *newStorage = newStorageArray; // point to the new storage space
    unsigned short temp[1];
	while (instream.eof() == false) {

		inputStorage_size++; // since we have a new input

		// Create a new array with the proper space to store the current number of inputs.
		newStorage = new unsigned short[inputStorage_size];

		// Load the data from the old storage array to the new
		for (int i = 0; i < inputStorage_size - 1; i++) {
			newStorage[i] = inputStorage[i];
		}

		instream >> temp[0];
		// Prevents an occasional bug where the last value would be copied in twice. Since
		// the same value shouldn't be called in twice anyway (especially one right after the
		// other) we will just prevent the entire scenario.
		if (inputStorage_size > 1 && newStorage[inputStorage_size - 2] == temp[0]) {
			inputStorage_size--;
		}
		else {
			newStorage[inputStorage_size - 1] = temp[0]; // Load the new input into the new storage
			inputStorage = newStorage; // Make sure the global pointer is pointing to the new array.
		}

		f++;
	}
	instream.close();

	/*
	// For use in debugging to ensure proper copying input.
	for (int i = 0; i < inputStorage_size; i++) {
		printf("%u\n", inputStorage[i]);
	}
	*/

}

// Retrieves the page number of the parameter passed logical address by retrieving the first
// 8 bits of the 16-bit address.
unsigned char getMSB(unsigned short logicalAddress) {
	int shiftNum = ADDRESS_BYTES * 4; // How many bits to shift.
	unsigned char MSB; // Where we will store the page number

	logicalAddress = logicalAddress >> shiftNum; // Make the page number the LSB.
	MSB = logicalAddress; // Store the address as a char to cut off the MSB.

	return MSB;
}

// Retrieves the least significant 8-bits of the parameter passed 16-bit address. Based
// on the address passed, this value can either be the frame number or the offset.
unsigned char getLSB(unsigned short logicalAddress) {
	unsigned char LSB; // Where we will store the LSB.
	LSB = logicalAddress; // Store the address as a char to cut off the MSB.
	return LSB;
}

// Replaces the page number in the logical address to the frame number for the physical.
unsigned short createPhysAdd(unsigned short logicalAddress, unsigned char frameNum) {
	unsigned char offset = logicalAddress; // The offset of the logical address (cuts off the page)
	unsigned short output = frameNum; // Where the physical address will go
	int shiftNum = ADDRESS_BYTES * 4; // How many bits to shift.
	output = output << shiftNum; // Make the frame number the LSB.
	output = output | offset; // Make the LSB the frame number using OR operation
	return output;
}

// Checks all entries within the TLB for one with the same page number as the logical
// address passed by the parameters. Return the frame number of the value found if it is within
// the TLB, otherwise, return 65280 (1111 1111 0000 0000).
// NOTE: If the replacement algorithm for the TLB is least recently used, the page is
// found, move the value to the 0 index of the array and push all other values down one.
// If the replacement algorithm is first in first out, leave the array alone. Remember that
// if there is a TLB hit, increment the TLBhit global.
// NOTE: Since the frame number is a byte and the return is a short if there is a miss,
// the MSB of the return will not be 0.
unsigned short checkTLB(unsigned short logicalAddress) {

	unsigned char pageNum = getMSB(logicalAddress); // The page number of the logical address

	unsigned char TLBPage; // The page number of the current TLB entry
	unsigned short temp; // Used to change the order of array elements
	unsigned short temp2; // Used to change the order of array elements;
	unsigned short value = 65280;

	// Go through everything in the TLB
	for (int i = 0; i < TLB_index; i++) {

		TLBPage = getMSB(TLB[i]); // The page number of the current TLB entry

		// There was a TLB hit
		if (TLBPage == pageNum) {

			// Set the return to the frame number for the entry that had a hit.
			value = getLSB(TLB[i]);
			TLBhit++; // We got a TLB hit.

			// Move the entry to the top of the list if we are using LRU algorithm
			if (repAlgo == 2) {
				temp = TLB[0];
				TLB[0] = TLB[i];
				for (int j = 0; j < i; j++) {
					temp2 = TLB[j + 1];
					TLB[j + 1] = temp;
					temp = temp2;
				}
			}
			i = TLB_index; // We got a hit so we'll end the loop after this.
		}

	}

	return value;

}

// Updates the TLB with the page number of the address passed by the parameters, and the
// frame number also passed that is associated to the page (page first 8-bit and frame second)
// NOTE: The possible replacement algorithms for the TLB all require the new entry to be placed
// at index 0, dropping the last index of the TLB is maxed out.
void updateTLB(unsigned short logicalAddress, unsigned short frameNum) {

	// Create the new TLB entry
	int shiftNum = ADDRESS_BYTES * 4; // How many bits to shift.
	unsigned short newEntry = getMSB(logicalAddress); // Get the page number for the logical address.
	newEntry = newEntry << shiftNum; // Make the page number the MSB.
	newEntry = newEntry | frameNum; // Make the LSB the frame number using OR operation

	// If there is still space for new TLB entries
	if (TLB_index < TLB_ENTRIES) TLB_index++; // Increment the number of spaces taken

	// Add the new element to the head of the TLB array.
	unsigned short temp = TLB[0];
	for (int i = 0; i < TLB_index; i++) {
		TLB[i] = newEntry; // Set an entry
		newEntry = temp; // Prepare the next entry that could be set
		// If there'll be space for another entry on the next loop, store the old value there.
		if (i + 1 < TLB_ENTRIES - 1) temp = TLB[i+1];
	}

}

// Checks all entries within the page table for one with the same page number as the logical
// address passed by the parameters. Return the frame number of the value found if it is within
// the page table, otherwise, return 65280 (1111 1111 0000 0000).
// NOTE: Remember that if there is a page fault, increment the pageFaults global.
unsigned short checkPageTable(unsigned short logicalAddress) {

	unsigned char pageNum = getMSB(logicalAddress); // The page number of the logical address

	unsigned char tablePage; // The page number of the current page table entry
	unsigned short value = 65280;

	// Go through everything in the page table
	for (int i = 0; i < pageTable_size; i++) {

		tablePage = getMSB(pageTable[i]); // The page number of the current page table entry

		// There was a page table hit
		if (tablePage == pageNum) {

			// Set the return to the frame number for the entry that had a hit.
			value = getLSB(pageTable[i]);
			i = PAGETABLE_SIZE; // We got a hit so we'll end the loop after this.
		}

	}

	if (value == 65280) pageFaults++; // Didn't find it
	return value;

}

// Updates the page table with the page number of the address passed by the parameters, and
// the frame number also passed that is associated to the page (page first 8-bit and frame
// second).
// NOTE: Page table and physical memory of equal size so simply maintain the order by placing
// the new value at the position required to keep the page numbers within the page table in
// sequential order.
void updatePageTable(unsigned short logicalAddress, unsigned short frameNum) {

	// Create the new page table entry
	int shiftNum = ADDRESS_BYTES * 4; // How many bits to shift.
	unsigned short newEntry = getMSB(logicalAddress); // Get the page number for the logical address.
	newEntry = newEntry << shiftNum; // Make the page number the MSB.
	newEntry = newEntry | frameNum; // Make the LSB the frame number using OR operation

	// Because the VM is the same size as the physical memory, we can afford to maintain the
	// order of the page table array based on the page number, therefor, add the new element
	// to the space in the array where the page numerically fits.
	//pageTable[getMSB(newEntry)] = newEntry;

	// Add the new entry to the next available space in the page table (since the page table and physical
	// memory have the same number of frame and pages, we assume that during normal execution the number
	// of pages cannot exceed the number of spaces for page table entries, therefore, we do not implement
	// a check to be sure the new entry exceeds the size of the array.
	pageTable[pageTable_size] = newEntry;
	pageTable_size++;

}

// Create the page table
// NOTE: Method by default assigns the same page and frame numbers to each index in sequential
// order due to the physical memory being the same size.
void createPageTable() {

	int shiftNum = ADDRESS_BYTES * 4; // How many bits to shift.
	unsigned short address; // The address for the entry
	unsigned short temp; // Holds the 8-bit pieces to be added to the entry

	// Create each page table entry one at a time
	for (int i = 0; i < PAGE_ENTRIES; i++) {

		// Create the entry for the current index
		address = i; // The page for the entry
		address = address << shiftNum; // Make the page number the MSB
		temp = i; // store the index as a char for OR operation
		address = address | temp; // Make the LSB the frame number using OR operation
		pageTable[i] = address; // add the address to the page table
		address = 0; // Reset the address for future loops
	}

	/*
	// For use during debugging
	for (int i = 0; i < PAGETABLE_SIZE; i++) {
		printf("%u\n", pageTable[i]);
	}
	*/

}

// Retrieve the value at the specified offset of the specified offset passed by the logicalAddress
// and the specified frame passed by the frameNumber and return it.
// NOTE: If this function is called after a page fault, make sure loadFromBacking()
// has been called first. Also, within the scope of this project, we can assume that the
// frame number is equal to the page number, therefore, on a page fault, set the passed
// frameNumber equal to the page number of the logicalAddress when calling this function.
unsigned char checkPhysMem(unsigned short physicalAddress) {

	unsigned char offset = getLSB(physicalAddress); // The offset of the parameter address
	unsigned char frameNum = getMSB(physicalAddress); // The frame number of the parameter address
	unsigned char value = physMem[frameNum * offset]; // The value within the physical memory

	return value;

}

// Creates the physical memory array. Note that within the span of this project, the physical
// memory should be equal to the BACKING_STORE array (except for testing).
void createPhysMem() {

		BACKING_STORE = fopen("BACKING_STORE", "r"); // should always be this

		fseek(BACKING_STORE, 0, SEEK_SET);

		fread(physMem, 1, PHYSMEM_SIZE, BACKING_STORE);

		fclose(BACKING_STORE); // remember to close it


		/*
		// Used for debugging to confirm correct input
		for (int i = 0; i < PHYSMEM_SIZE; i++) {
			printf("%u\n", physMem[i]);
		}*/

}

// Used if the user wants to create the physical memory from a specific file.
void createPhysMem(char fileName[]) {
	// Read the file
	instream.open(fileName);

	// Read in the numbers from the file.
    int f = 0;
    unsigned char temp[1];
	while (instream.eof() == false) {
		instream >> temp[0];

		physMem[f] = getMSB(temp[0]);
		f++;
	}
	instream.close();


	// For use in debugging to ensure proper copying input.
	/*
	for (int i = 0; i < f; i++) {
		printf("%u\n", physMem[i]);
	}
	*/

}

// Pulls the data for the parameter specified page from the BACKING_STORE and stores it
// into the physical memory (starting from the same offset as where it is pulled from
// BACKING_STORE since they are the same size. Returns the frame that was updated
void loadFromBacking(unsigned short frameNum) {

	BACKING_STORE = fopen("BACKING_STORE", "r"); // should always be this

	int offset = frameNum * PAGE_SIZE;
	char tempStorage[PAGE_SIZE];
	fseek(BACKING_STORE, offset, SEEK_SET);

	fread(tempStorage, 1, PAGE_SIZE, BACKING_STORE);

	fclose(BACKING_STORE); // remember to close it

	// Load the data read in from the BACKING_STORE to the physical memory
	for (int i = 0; i < PAGE_SIZE; i++) {
		physMem[offset + i] = tempStorage[i];
	}

	/*
	// Used for debugging to confirm correct input
	for (int i = 0; i < PAGE_SIZE; i++) {
		printf("TempStorage: %u\n", tempStorage[i]);
		printf("PhysMem: %u\n", physMem[offset + i]);
	}
	*/

}

// TEST1: Tests the system on page faults (takes test1.txt)
// Sets the stage for test1 which alters the first, last, and a random middle page entry in the page table
// for tests involving forcing page faults. (official test1.txt file should guarantee 100 percent page faults)
void test1() {
	// Change the page table entries (DEFUNCT: This worked when I thought we loaded in the full physical memory on startup)
	//pageTable[0] = 1280; // page 0 is equal to page 5 (so two page 5 entries no page 0 entries)
	//printf("Test 1: pageTable[0] (Initial): %u\n", pageTable[0]);
	//pageTable[PAGETABLE_SIZE - 1] = 2560; // page 255 is equal to page 10 (so two page 10 entries no page 255 entries)
	//printf("Test 1: pageTable[255] (Initial): %u\n", pageTable[PAGETABLE_SIZE - 1]);
	//pageTable[15] = 6400; // page 15 is equal to page 25 (so two page 25 entries no page 15 entries)
	//printf("Test 1: pageTable[15] (Initial): %u\n", pageTable[15]);

	// Change the physical memory data to confirm the data is reacquired
	physMem[0] = 255;
	printf("Test 1: physMem[0] (Initial): %u\n", physMem[0]);
	physMem[(PAGETABLE_SIZE - 1) * PAGE_SIZE] = 255;
	printf("Test 1: physMem[255] (Initial): %u\n", physMem[(PAGETABLE_SIZE - 1) * PAGE_SIZE]);
	physMem[15 * PAGE_SIZE] = 255;
	printf("Test 1: physMem[15] (Initial): %u\n", physMem[15 * PAGE_SIZE]);
}

// Shows the results of test1 after full execution of the system.
void test1Results() {
	printf("\nTest 1: pageTable[0] (Expected: 0): %u\n", pageTable[0]);
	//printf("Test 1: pageTable[5] (Expected: 1285): %u\n", pageTable[5]);
	printf("Test 1: pageTable[1] (Expected: 65535): %u\n", pageTable[1]);
	printf("Test 1: pageTable[2] (Expected: 3855): %u\n", pageTable[2]);
	printf("Test 1: physMem[0] (Expected: 0): %u\n", physMem[0]);
	printf("Test 1: physMem[255] (Expected: 0): %u\n", physMem[(PAGETABLE_SIZE - 1) * PAGE_SIZE]);
	printf("Test 1: physMem[15] (Expected: 0): %u\n", physMem[15 * PAGE_SIZE]);
}

// TEST 2: Tests the system's handling of the TLB (takes test2.txt)
// Prints the results of test2
// Tests the handling of the TLB by first completely filling the TLB, then testing the replacement algorithm by adding
// another entry. The system then attempts to test the reordering of the TLB by searching for a page in the center of the
// TLB. Also tests the TLB hit rate counter (only 1 hit when using an unaltered test2.txt file, 18 inputs overall).
void test2Results() {
	printf("\nTest 2: TLB[0] (Expected: FIFO: 4112, LRU: 2056): %u\n", TLB[0]);
	printf("Test 2: TLB[1] (Expected: FIFO: 3855, LRU: 4112): %u\n", TLB[1]);
	printf("Test 2: TLB[2] (Expected: FIFO: 3598, LRU: 3855): %u\n", TLB[2]);
	printf("Test 2: TLB[3] (Expected: FIFO: 3341, LRU: 3598): %u\n", TLB[3]);
	printf("Test 2: TLB[4] (Expected: FIFO: 3084, LRU: 3341): %u\n", TLB[4]);
	printf("Test 2: TLB[5] (Expected: FIFO: 2827, LRU: 3084): %u\n", TLB[5]);
	printf("Test 2: TLB[6] (Expected: FIFO: 2570, LRU: 2827): %u\n", TLB[6]);
	printf("Test 2: TLB[7] (Expected: FIFO: 2313, LRU: 2570): %u\n", TLB[7]);
	printf("Test 2: TLB[8] (Expected: FIFO: 2056, LRU: 2313): %u\n", TLB[8]);
	printf("Test 2: TLB[9] (Expected: FIFO: 1799, LRU: 1799): %u\n", TLB[9]);
	printf("Test 2: TLB[10] (Expected: FIFO: 1542, LRU: 1542): %u\n", TLB[10]);
	printf("Test 2: TLB[11] (Expected: FIFO: 1285, LRU: 1285): %u\n", TLB[11]);
	printf("Test 2: TLB[12] (Expected: FIFO: 1028, LRU: 1028): %u\n", TLB[12]);
	printf("Test 2: TLB[13] (Expected: FIFO: 771, LRU: 771): %u\n", TLB[13]);
	printf("Test 2: TLB[14] (Expected: FIFO: 514, LRU: 514): %u\n", TLB[14]);
	printf("Test 2: TLB[15] (Expected: FIFO: 257, LRU: 257): %u\n", TLB[15]);
}
