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

struct StreamBuffer {
    bool valid;
    std::vector<uint32_t> buffer;
    int lastAccessedIndex;

    StreamBuffer(int size) : valid(false), buffer(size), lastAccessedIndex(-1) {}

    bool access(uint32_t address) {
        auto it = std::find(buffer.begin(), buffer.end(), address);
        if (it != buffer.end()) {
            lastAccessedIndex = std::distance(buffer.begin(), it);
            return true;
        }
        return false;
    }

    void prefetch(uint32_t address) {
        valid = true;
        lastAccessedIndex = 0;
        buffer[0] = address;
        for (int i = 1; i < buffer.size(); ++i) {
            buffer[i] = address + i;
        }
    }

    void shift() {
        if (lastAccessedIndex >= 0) {
            buffer.erase(buffer.begin(), buffer.begin() + lastAccessedIndex + 1);
            buffer.resize(buffer.size(), 0);
            lastAccessedIndex = -1;
        }
    }
};

class Cache {
public:
    Cache(int size, int associativity, int blockSize, bool prefetchEnabled, int prefetchDistance)
    : size(size), associativity(associativity), blockSize(blockSize),
      prefetchEnabled(prefetchEnabled), streamBuffer(prefetchDistance) {
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

        // Check if the requested block is already present in the cache
        for (int i = 0; i < associativity; ++i) {
            if (set.blocks[i].valid && set.blocks[i].tag == tag) {
                // std::cout << "Cache hit: " << std::hex << address << std::dec << std::endl;
                if (isWrite) {
                    set.blocks[i].dirty = true;
                }
                updateLRU(set, i);
                return true;
            }
        }

        // Check stream buffer
        if (streamBuffer.access(address)) {
            // Stream buffer hit
            cout << "Stream buffer hit: " << hex << address << dec << endl;
            if (!isWrite) {
                // Shift out accessed blocks
                streamBuffer.shift();
            }
            return true;
        }

        // Miss
        // std::cout << "Cache miss: " << std::hex << address << std::dec << std::endl;
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

        // Check for prefetching opportunity
        if (!isWrite && prefetchEnabled && !streamBuffer.valid) {
            uint32_t prefetchAddress = address + 1;
            cout << "Prefetching address: " << hex << "0x" << prefetchAddress << dec << endl;
            if (nextLevel) {
                nextLevel->access(prefetchAddress, false, nullptr);
            }
            streamBuffer.prefetch(prefetchAddress);
        }

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

    StreamBuffer streamBuffer;
    bool prefetchEnabled;
};

class MemoryHierarchy {
public:
    MemoryHierarchy(int l1Size, int l1Associativity, int l2Size, int l2Associativity, int blockSize,
                    bool prefetchEnabled, int prefetchDistance)
        : l1(l1Size, l1Associativity, blockSize, prefetchEnabled, prefetchDistance),
          l2(l2Size, l2Associativity, blockSize, prefetchEnabled, prefetchDistance),
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
    bool prefetchEnabled = stoi(argv[6]) > 0;
    int prefetchDistance = stoi(argv[7]);
    string traceFile = argv[8];

    cout << "Block Size: " << blockSize << " bytes" << endl;
    cout << "L1 Size: " << l1Size << " bytes" << endl;
    cout << "L1 Associativity: " << l1Assoc << endl;
    cout << "L2 Size: " << l2Size << " bytes" << endl;
    cout << "L2 Associativity: " << l2Assoc << endl;
    cout << "Prefetching enabled: " << (prefetchEnabled ? "1 (enabled)" : "0 (disabled)") << endl;
    cout << "Prefetch distance: " << (prefetchEnabled ? std::to_string(prefetchDistance) : "0 (not applicable)") << endl;
    cout << "Trace File: " << traceFile << endl;

    MemoryHierarchy hierarchy(l1Size, l1Assoc, l2Size, l2Assoc, blockSize, prefetchEnabled, prefetchDistance);

    ifstream trace(traceFile);
    string line;
    while (getline(trace, line)) {
        bool isWrite = line[0] == 'w';
        uint32_t address = stoul(line.substr(2), nullptr, 16);
        hierarchy.access(address, isWrite);
    }

    hierarchy.printStats();
}