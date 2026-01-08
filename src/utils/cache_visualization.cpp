/**
 * @file cache_visualization.cpp
 * @brief Cache state visualization implementation
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 */

#include "cache_visualization.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace cachesim {

std::vector<CacheBlockState>
CacheVisualization::extractCacheState(const Cache &cache,
                                      [[maybe_unused]] int cacheLevel) {
  std::vector<CacheBlockState> result;

  uint32_t numSets = cache.getNumSets();
  uint32_t associativity = cache.getAssociativity();
  uint32_t blockSize = cache.getBlockSize();

  uint32_t offsetBits = static_cast<uint32_t>(std::log2(blockSize));
  uint32_t setBits = static_cast<uint32_t>(std::log2(numSets));

  for (uint32_t set = 0; set < numSets; ++set) {
    for (uint32_t way = 0; way < associativity; ++way) {
      if (cache.isBlockValid(set, way)) {
        CacheBlockState blockState;
        blockState.set = set;
        blockState.way = way;
        blockState.valid = true;
        blockState.dirty = cache.isBlockDirty(set, way);
        blockState.tag = cache.getBlockTag(set, way);
        blockState.accessCount = cache.getBlockAccessCount(set, way);
        blockState.lastAccess = cache.getBlockLastAccess(set, way);
        blockState.prefetched = cache.isBlockPrefetched(set, way);
        blockState.address =
            (blockState.tag << (offsetBits + setBits)) | (set << offsetBits);
        result.push_back(blockState);
      }
    }
  }

  return result;
}

std::string CacheVisualization::createVisualization(
    const std::vector<CacheBlockState> &blockStates, const Cache &cache,
    uint32_t maxBlocks, bool useColors, int cacheLevel) {
  std::ostringstream output;

  uint32_t numSets = cache.getNumSets();
  uint32_t associativity = cache.getAssociativity();
  uint32_t blockSize = cache.getBlockSize();

  std::string resetColor = useColors ? "\033[0m" : "";
  std::string headerColor = useColors ? "\033[1;36m" : "";
  std::string validColor = useColors ? "\033[1;32m" : "";
  std::string dirtyColor = useColors ? "\033[1;33m" : "";
  std::string addressColor = useColors ? "\033[1;34m" : "";
  std::string prefetchColor = useColors ? "\033[1;35m" : "";

  auto sortedBlocks = blockStates;
  std::sort(sortedBlocks.begin(), sortedBlocks.end(),
            [](const CacheBlockState &a, const CacheBlockState &b) {
              if (a.set != b.set)
                return a.set < b.set;
              return a.way < b.way;
            });

  if (maxBlocks > 0 && sortedBlocks.size() > maxBlocks) {
    sortedBlocks.resize(maxBlocks);
  }

  const char *boxTL = "+";
  const char *boxTR = "+";
  const char *boxBL = "+";
  const char *boxBR = "+";
  const char *boxH = "-";
  const char *boxV = "|";
  const char *boxT = "+";

  const int TABLE_WIDTH = 77;

  output << headerColor << boxTL;
  for (int i = 0; i < TABLE_WIDTH - 2; i++)
    output << boxH;
  output << boxTR << resetColor << std::endl;

  std::string title = " L" + std::to_string(cacheLevel) + " Cache State ";
  int titlePadLeft = (TABLE_WIDTH - 2 - static_cast<int>(title.length())) / 2;
  int titlePadRight =
      TABLE_WIDTH - 2 - static_cast<int>(title.length()) - titlePadLeft;
  output << headerColor << boxV << std::string(titlePadLeft, ' ') << title
         << std::string(titlePadRight, ' ') << boxV << resetColor << std::endl;

  output << headerColor << boxT;
  for (int i = 0; i < TABLE_WIDTH - 2; i++)
    output << boxH;
  output << boxT << resetColor << std::endl;

  output << headerColor << boxV
         << " Set | Way |     Tag     | Valid | Dirty |   Address   | Access | "
            "Pref "
         << boxV << resetColor << std::endl;

  output << headerColor << boxT;
  for (int i = 0; i < TABLE_WIDTH - 2; i++)
    output << boxH;
  output << boxT << resetColor << std::endl;

  for (const auto &block : sortedBlocks) {
    output << headerColor << boxV << " " << resetColor;
    output << std::setw(3) << block.set << headerColor << " | " << resetColor;
    output << std::setw(3) << block.way << headerColor << " | " << resetColor;
    output << addressColor << "0x" << std::hex << std::setw(9)
           << std::setfill('0') << block.tag << std::dec << std::setfill(' ')
           << headerColor << " | " << resetColor;
    output << validColor << std::setw(5) << "Yes" << headerColor << " | "
           << resetColor;
    if (block.dirty) {
      output << dirtyColor << std::setw(5) << "Yes";
    } else {
      output << std::setw(5) << "No";
    }
    output << headerColor << " | " << resetColor;
    output << addressColor << "0x" << std::hex << std::setw(9)
           << std::setfill('0') << block.address << std::dec
           << std::setfill(' ') << headerColor << " | " << resetColor;
    output << std::setw(6) << block.accessCount << headerColor << " | "
           << resetColor;
    if (block.prefetched) {
      output << prefetchColor << std::setw(4) << "Yes";
    } else {
      output << std::setw(4) << "No";
    }
    output << headerColor << " " << boxV << resetColor << std::endl;
  }

  if (sortedBlocks.empty()) {
    output << headerColor << boxV << std::setw(75)
           << "Cache is empty or all blocks are invalid"
           << " " << boxV << resetColor << std::endl;
  } else if (maxBlocks > 0 && blockStates.size() > maxBlocks) {
    output << headerColor << boxV << std::setw(TABLE_WIDTH - 3)
           << "... (showing first " + std::to_string(maxBlocks) + " blocks)"
           << " " << boxV << resetColor << std::endl;
  }

  output << headerColor << boxT;
  for (int i = 0; i < TABLE_WIDTH - 2; i++)
    output << boxH;
  output << boxT << resetColor << std::endl;

  size_t dirtyCount =
      std::count_if(blockStates.begin(), blockStates.end(),
                    [](const CacheBlockState &b) { return b.dirty; });
  std::ostringstream configStr, validStr, dirtyStr;
  configStr << " Config: " << numSets << " sets x " << associativity
            << " ways x " << blockSize << " bytes";
  validStr << " Valid: " << blockStates.size() << "/"
           << (numSets * associativity) << " blocks (" << std::fixed
           << std::setprecision(1)
           << (blockStates.size() * 100.0 / (numSets * associativity)) << "%)";
  dirtyStr << " Dirty: " << dirtyCount << "/" << blockStates.size()
           << " blocks (" << std::fixed << std::setprecision(1)
           << (blockStates.empty() ? 0.0
                                   : (dirtyCount * 100.0 / blockStates.size()))
           << "%)";

  output << headerColor << boxV << resetColor << std::left
         << std::setw(TABLE_WIDTH - 2) << configStr.str() << headerColor << boxV
         << resetColor << std::endl;
  output << headerColor << boxV << resetColor << std::left
         << std::setw(TABLE_WIDTH - 2) << validStr.str() << headerColor << boxV
         << resetColor << std::endl;
  output << headerColor << boxV << resetColor << std::left
         << std::setw(TABLE_WIDTH - 2) << dirtyStr.str() << headerColor << boxV
         << resetColor << std::endl;

  output << headerColor << boxBL;
  for (int i = 0; i < TABLE_WIDTH - 2; i++)
    output << boxH;
  output << boxBR << resetColor << std::endl;

  return output.str();
}

std::string CacheVisualization::createSummary(const Cache &cache,
                                              int cacheLevel) {
  std::ostringstream output;
  uint32_t cacheSize =
      cache.getNumSets() * cache.getAssociativity() * cache.getBlockSize();
  output << "L" << cacheLevel << " Cache: " << cache.getNumSets() << " sets x "
         << cache.getAssociativity() << " ways x " << cache.getBlockSize()
         << "B = " << (cacheSize / 1024) << "KB";
  return output.str();
}

} // namespace cachesim
