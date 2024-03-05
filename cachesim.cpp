#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cmath>

using namespace std;

struct CacheBlock {
    bool valid;
    bool dirty;
    uint32_t tag;
    vector<uint8_t> data;
};

struct CacheSet {
    vector<CacheBlock> blocks;
    vector<int> lruOrder;
};

class Cache {
public:
    Cache(int size, int associativity, int blockSize) : 
        size(size), associativity(associativity), blockSize(blockSize) {
        numSets = size / (associativity * blockSize);
        sets.resize(numSets);
        for (auto& set : sets) {
            set.blocks.resize(associativity);
            set.lruOrder.resize(associativity);
            for (int i = 0; i < associativity; ++i) {
                set.lruOrder[i] = i;
            }
        }
    }

    bool access(uint32_t address, bool isWrite, Cache* nextLevel = nullptr) {
        uint32_t tag = address / blockSize;
        int setIndex = (address / blockSize) % numSets;
        auto& set = sets[setIndex];

        for (int i = 0; i < associativity; ++i) {
            if (set.blocks[i].valid && set.blocks[i].tag == tag) {
                // Hit
                if (isWrite) {
                    set.blocks[i].dirty = true;
                }
                updateLRU(set, i);
                return true;
            }
        }

        // Miss
        int victimIndex = set.lruOrder.back();
        auto& victimBlock = set.blocks[victimIndex];

        if (victimBlock.valid && victimBlock.dirty) {
            // Write back victim block to next level
            uint32_t victimAddress = (victimBlock.tag * numSets + setIndex) * blockSize;
            writeBack(victimAddress, nextLevel);
        }

        // Bring in new block
        uint32_t blockAddress = (tag * numSets + setIndex) * blockSize;
        if (nextLevel) {
            nextLevel->access(blockAddress, false, nullptr);
        }
        victimBlock = {true, isWrite, tag};

        updateLRU(set, victimIndex);
        return false;
    }

private:
    void updateLRU(CacheSet& set, int accessedIndex) {
        auto it = find(set.lruOrder.begin(), set.lruOrder.end(), accessedIndex);
        set.lruOrder.erase(it);
        set.lruOrder.insert(set.lruOrder.begin(), accessedIndex);
    }

    void writeBack(uint32_t address, Cache* nextLevel) {
        if (nextLevel) {
            nextLevel->access(address, true, nullptr);
        }
        // Write to main memory
    }

    int size;
    int associativity;
    int blockSize;
    int numSets;
    vector<CacheSet> sets;
};

class MemoryHierarchy {
public:
    MemoryHierarchy(int l1Size, int l1Associativity, int l2Size, int l2Associativity, int blockSize) :
        l1(l1Size, l1Associativity, blockSize),
        l2(l2Size, l2Associativity, blockSize),
        blockSize(blockSize),
        l2Size(l2Size) {}

    void access(uint32_t address, bool isWrite) {
        if (!l1.access(address, isWrite, l2Size > 0 ? &l2 : nullptr)) {
            // L1 miss
            l1Misses++;
            if (l2Size > 0) {
                l2.access(address, isWrite, nullptr);
            }
        }
        if (isWrite) {
            l1Writes++;
        } else {
            l1Reads++;
        }
    }

    void printStats() {
        cout << "L1 reads: " << l1Reads << endl;
        cout << "L1 writes: " << l1Writes << endl;
        cout << "L1 misses: " << l1Misses << endl;
        // Print other stats
    }

private:
    Cache l1;
    Cache l2;
    int blockSize;
    int l2Size;
    int l1Reads = 0;
    int l1Writes = 0;
    int l1Misses = 0;
    // Other stats
};

int main(int argc, char* argv[]) {
    if (argc != 9) {
        cerr << "Usage: " << argv[0] << " <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <PREF_N> <PREF_M> <trace_file>" << endl;
        return 1;
    }

    int blockSize = stoi(argv[1]);
    int l1Size = stoi(argv[2]);
    int l1Assoc = stoi(argv[3]);
    int l2Size = stoi(argv[4]);
    int l2Assoc = stoi(argv[5]);
    // int prefN = stoi(argv[6]);
    // int prefM = stoi(argv[7]);
    string traceFile = argv[8];

    // cout << "Block Size: " << blockSize << endl;
    // cout << "L1 Size: " << l1Size << endl;
    // cout << "L1 Associativity: " << l1Assoc << endl;
    // cout << "L2 Size: " << l2Size << endl;
    // cout << "L2 Associativity: " << l2Assoc << endl;
    // cout << "Trace File: " << traceFile << endl;

    MemoryHierarchy hierarchy(l1Size, l1Assoc, l2Size, l2Assoc, blockSize);

    ifstream trace(traceFile);
    string line;
    while (getline(trace, line)) {
        bool isWrite = line[0] == 'w';
        uint32_t address = stoul(line.substr(2), nullptr, 16);
        hierarchy.access(address, isWrite);
    }

    hierarchy.printStats();
}