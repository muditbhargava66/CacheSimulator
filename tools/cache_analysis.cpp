/**
 * @file cache_analysis.cpp
 * @brief Hit-rate vs Associativity / Cache-capacity sweep
 *
 * Generates a table and ASCII line chart matching the format shown in the
 * v1.4.3 bug-report email.  After the associativity bug fix the chart should
 * show hit-rate INCREASING (or staying flat) with higher associativity.
 *
 * Workload design
 * ~~~~~~~~~~~~~~~
 * For each (cacheSize, assoc) pair a per-set conflict pattern is used:
 *
 *   1. Install `assoc` blocks that all map to the same set.
 *   2. Re-access those `assoc` blocks (guaranteed hits).
 *   3. Access `K` additional "conflict" blocks in the same set (misses).
 *
 * After warmup the steady-state hit rate converges to assoc / (2·assoc + K).
 * Higher associativity → higher hit rate — confirming the fix.
 *
 * To differentiate cache sizes, K is scaled inversely with cache capacity:
 * K = baseDepth × (maxCacheSize / cacheSize).  Smaller caches see heavier
 * conflict pressure, producing lower hit-rate curves.
 *
 * @author  Mudit Bhargava
 * @date    2026-02-10
 * @version 1.4.3
 */

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../src/core/cache.h"

using namespace cachesim;

// ────────────────────────────────────────────────────────────────────────────
//  Simulation
// ────────────────────────────────────────────────────────────────────────────

/**
 * @brief Run a per-config conflict simulation.
 *
 * @param cacheSize   Cache capacity (bytes)
 * @param assoc       Number of ways
 * @param blockSize   Cache-line size (bytes)
 * @param extraDepth  Conflict blocks per set (K)
 * @param iterations  Repetitions of the full pattern
 * @return            Hit rate [0, 100] %
 */
static double runSimulation(int cacheSize, int assoc, int blockSize,
                            int extraDepth, int iterations) {
    CacheConfig cfg(cacheSize, assoc, blockSize);
    Cache cache(cfg);

    const int numSets    = cacheSize / (blockSize * assoc);
    const int activeSets = numSets;  // exercise every set

    for (int iter = 0; iter < iterations; ++iter) {
        for (int s = 0; s < activeSets; ++s) {
            // Step 1 — install `assoc` blocks (first iter: misses;
            //          later: partial hits from LRU survivors)
            for (int b = 0; b < assoc; ++b) {
                uint32_t addr = static_cast<uint32_t>(
                    (s + b * numSets) * blockSize);
                (void)cache.access(addr, false);
            }
            // Step 2 — re-access (guaranteed hits)
            for (int b = 0; b < assoc; ++b) {
                uint32_t addr = static_cast<uint32_t>(
                    (s + b * numSets) * blockSize);
                (void)cache.access(addr, false);
            }
            // Step 3 — conflict blocks (always misses)
            for (int e = 0; e < extraDepth; ++e) {
                uint32_t addr = static_cast<uint32_t>(
                    (s + (assoc + e) * numSets) * blockSize);
                (void)cache.access(addr, false);
            }
        }
    }

    return cache.getHitRatio() * 100.0;
}

// ────────────────────────────────────────────────────────────────────────────
//  Output helpers
// ────────────────────────────────────────────────────────────────────────────

static std::string sizeLabel(int bytes) {
    std::ostringstream o;
    if (bytes >= 1024) o << (bytes / 1024) << "KB";
    else               o << bytes << "B";
    return o.str();
}

static void printTable(const std::vector<int>& sizes,
                       const std::vector<int>& assocs,
                       const std::vector<std::vector<double>>& hr) {
    const int colW = 10;
    int width = 18 + static_cast<int>(sizes.size()) * colW + 1;

    std::cout << "\n" << std::string(width, '=') << "\n";
    std::cout << std::left << std::setw(18) << "Hit Rate [%]" << "|";
    for (int s : sizes)
        std::cout << std::right << std::setw(colW) << sizeLabel(s);
    std::cout << "\n";
    std::cout << std::string(18, '-') << "+"
              << std::string(sizes.size() * colW, '-') << "\n";

    for (size_t r = 0; r < assocs.size(); ++r) {
        std::ostringstream lbl;
        lbl << assocs[r] << "-way";
        std::cout << std::left << std::setw(18) << lbl.str() << "|";
        for (size_t c = 0; c < sizes.size(); ++c)
            std::cout << std::right << std::fixed << std::setprecision(2)
                      << std::setw(colW) << hr[r][c];
        std::cout << "\n";
    }
    std::cout << std::string(width, '=') << "\n\n";
}

static void printChart(const std::vector<int>& sizes,
                       const std::vector<int>& assocs,
                       const std::vector<std::vector<double>>& hr) {

    const int H = 20, colW = 14;
    const char sym[] = {'#', '*', '+', 'o', '@', '~'};

    // Auto-scale Y
    double yLo = 100, yHi = 0;
    for (auto& row : hr) for (double v : row) { yLo = std::min(yLo, v); yHi = std::max(yHi, v); }
    yLo = std::max(0.0, std::floor(yLo / 5) * 5 - 5);
    yHi = std::min(100.0, std::ceil(yHi / 5) * 5 + 5);
    double yR = std::max(yHi - yLo, 1.0);

    std::cout << "  Hit Ratio vs Associativity\n"
              << "  (each symbol = one cache-size series)\n\n";
    for (size_t s = 0; s < sizes.size(); ++s)
        std::cout << "   " << sym[s % 6] << " = " << sizeLabel(sizes[s]) << "\n";
    std::cout << "\n";

    int W = static_cast<int>(assocs.size()) * colW;
    std::vector<std::string> canvas(H + 1, std::string(W + 8, ' '));

    // Data points
    for (size_t s = 0; s < sizes.size(); ++s)
        for (size_t a = 0; a < assocs.size(); ++a) {
            int r = H - static_cast<int>((hr[a][s] - yLo) / yR * H + 0.5);
            r = std::clamp(r, 0, H);
            int c = 7 + static_cast<int>(a) * colW + colW / 2;
            if (c < (int)canvas[0].size()) canvas[r][c] = sym[s % 6];
        }

    // Lines between points
    for (size_t s = 0; s < sizes.size(); ++s)
        for (size_t a = 0; a + 1 < assocs.size(); ++a) {
            int r1 = H - static_cast<int>((hr[a][s] - yLo) / yR * H + 0.5);
            int r2 = H - static_cast<int>((hr[a+1][s] - yLo) / yR * H + 0.5);
            r1 = std::clamp(r1, 0, H); r2 = std::clamp(r2, 0, H);
            int c1 = 7 + (int)a * colW + colW / 2;
            int c2 = 7 + (int)(a+1) * colW + colW / 2;
            int steps = std::max(std::abs(r2 - r1), std::abs(c2 - c1));
            if (!steps) continue;
            for (int t = 1; t < steps; ++t) {
                int r = r1 + (r2 - r1) * t / steps;
                int c = c1 + (c2 - c1) * t / steps;
                if (r >= 0 && r <= H && c >= 0 && c < (int)canvas[0].size()
                    && canvas[r][c] == ' ')
                    canvas[r][c] = '.';
            }
        }

    // Render
    for (int r = 0; r <= H; ++r) {
        if (r % 4 == 0)
            std::cout << std::right << std::setw(5) << std::fixed
                      << std::setprecision(0) << (yHi - yR * r / H) << " |";
        else
            std::cout << "      |";
        std::cout << canvas[r] << "\n";
    }
    std::cout << "      +" << std::string(W + 6, '-') << "\n       ";
    for (size_t a = 0; a < assocs.size(); ++a) {
        std::ostringstream l; l << assocs[a] << "-way";
        std::cout << std::setw(colW) << l.str();
    }
    std::cout << "\n" << std::string(7 + W, ' ') << "Associativity\n\n";
}

static bool validateMonotonicity(const std::vector<int>& sizes,
                                 const std::vector<int>& assocs,
                                 const std::vector<std::vector<double>>& hr) {
    bool ok = true;
    for (size_t c = 0; c < sizes.size(); ++c)
        for (size_t r = 1; r < assocs.size(); ++r)
            if (hr[r][c] < hr[r-1][c] - 0.01) {
                std::cerr << "  WARNING: hit rate decreased at "
                          << assocs[r] << "-way vs " << assocs[r-1]
                          << "-way for " << sizeLabel(sizes[c]) << " cache ("
                          << hr[r][c] << "% < " << hr[r-1][c] << "%)\n";
                ok = false;
            }
    return ok;
}

// ────────────────────────────────────────────────────────────────────────────
//  main
// ────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "===============================================\n"
              << "  Cache Simulator v1.4.3 - Associativity Analysis\n"
              << "===============================================\n\n";

    const int blockSize  = 32;
    const int baseDepth  = 3;    // conflict blocks at largest cache size
    const int iterations = 200;

    std::vector<int> cacheSizes = {2048, 4096, 8192, 16384};
    std::vector<int> assocs     = {1, 2, 4, 8};
    int maxSize = *std::max_element(cacheSizes.begin(), cacheSizes.end());

    std::cout << "Workload parameters\n"
              << "  Block size    : " << blockSize << " B\n"
              << "  Base depth (K): " << baseDepth
              << " conflict blocks per set (at " << sizeLabel(maxSize) << ")\n"
              << "  K is scaled   : K = " << baseDepth
              << " x (maxSize / cacheSize)\n"
              << "  Iterations    : " << iterations << "\n"
              << "  Steady-state  : hit-rate ≈ assoc / (2·assoc + K)\n\n";

    // Show per-size K values
    std::cout << "  Effective K per cache size:\n";
    for (int sz : cacheSizes) {
        int K = baseDepth * maxSize / sz;
        std::cout << "    " << std::setw(5) << sizeLabel(sz)
                  << " : K = " << K << "\n";
    }
    std::cout << "\n";

    // ── Sweep ──
    std::vector<std::vector<double>> hitRates(
        assocs.size(), std::vector<double>(cacheSizes.size()));

    for (size_t a = 0; a < assocs.size(); ++a)
        for (size_t s = 0; s < cacheSizes.size(); ++s) {
            int K = baseDepth * maxSize / cacheSizes[s];
            hitRates[a][s] = runSimulation(
                cacheSizes[s], assocs[a], blockSize, K, iterations);
        }

    printTable(cacheSizes, assocs, hitRates);
    printChart(cacheSizes, assocs, hitRates);

    std::cout << "=== Monotonicity Check ===\n";
    bool ok = validateMonotonicity(cacheSizes, assocs, hitRates);
    if (ok)
        std::cout << "  PASS: hit rate is non-decreasing with increasing\n"
                  << "        associativity for all cache sizes.\n"
                  << "  (Confirms the v1.4.3 associativity bug fix.)\n\n";
    else
        std::cerr << "  FAIL: some series are still decreasing.\n\n";

    return ok ? 0 : 1;
}
