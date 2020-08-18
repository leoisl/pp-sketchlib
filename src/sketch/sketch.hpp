/*
 *
 * sketch.hpp
 * bindash sketch method
 *
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <tuple>

#include "seqio.hpp"

std::tuple<std::vector<uint64_t>, double, bool> sketch(SeqBuf &seq,
                                                        const uint64_t sketchsize,
                                                        const size_t kmer_len,
                                                        const size_t bbits,
                                                        const bool use_canonical = true,
                                                        const uint8_t min_count = 0,
                                                        const bool exact = false);

#ifdef GPU_AVAILABLE
class GPUCountMin;

std::tuple<robin_hood::unordered_map<int, std::vector<uint64_t>>, size_t, bool>
   sketch_gpu(
        SeqBuf &seq,
        GPUCountMin &countmin,
        const uint64_t sketchsize,
        const std::vector<size_t>& kmer_lengths,
        const size_t bbits,
        const bool use_canonical,
        const uint8_t min_count,
        const size_t cpu_threads
    );
#endif