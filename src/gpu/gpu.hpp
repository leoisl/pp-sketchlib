/*
 *
 * gpu.hpp
 * functions using CUDA
 *
 */
#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

#include "reference.hpp"

static const int warp_size = 32;

// Align structs
// https://stackoverflow.com/a/12779757
#if defined(__CUDACC__) // NVCC
   #define ALIGN(n) __align__(n)
#elif defined(__GNUC__) // GCC
  #define ALIGN(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER) // MSVC
  #define ALIGN(n) __declspec(align(n))
#else
  #error "Please provide a definition for MY_ALIGN macro for your host compiler!"
#endif

struct ALIGN(8) RandomStrides {
	size_t kmer_stride;
	size_t cluster_inner_stride;
	size_t cluster_outer_stride;
};

typedef std::tuple<RandomStrides, std::vector<float>> FlatRandom;

#ifdef GPU_AVAILABLE
// Structure of flattened vectors
struct ALIGN(16) SketchStrides {
	size_t bin_stride;
	size_t kmer_stride;
	size_t sample_stride;
	size_t sketchsize64;
	size_t bbits;
};

struct ALIGN(8) SketchSlice {
	size_t ref_offset;
	size_t ref_size;
	size_t query_offset;
	size_t query_size;
};

// defined in cuda.cuh
std::tuple<size_t, size_t> initialise_device(const int device_id);

// defined in dist.cu
std::vector<uint64_t> flatten_by_bins(
	const std::vector<Reference>& sketches,
	const std::vector<size_t>& kmer_lengths,
	SketchStrides& strides,
	const size_t start_sample_idx,
	const size_t end_sample_idx,
  const int cpu_threads = 1);

std::vector<uint64_t> flatten_by_samples(
	const std::vector<Reference>& sketches,
	const std::vector<size_t>& kmer_lengths,
	SketchStrides& strides,
	const size_t start_sample_idx,
	const size_t end_sample_idx,
  const int cpu_threads = 1);

std::vector<float> dispatchDists(
				   std::vector<Reference>& ref_sketches,
				   std::vector<Reference>& query_sketches,
				   SketchStrides& ref_strides,
				   SketchStrides& query_strides,
				   const FlatRandom& flat_random,
				   const std::vector<uint16_t>& ref_random_idx,
				   const std::vector<uint16_t>& query_random_idx,
				   const SketchSlice& sketch_subsample,
				   const std::vector<size_t>& kmer_lengths,
				   const bool self,
           const int cpu_threads);

// Memory on device for each operation
class DeviceMemory {
	public:
		DeviceMemory(SketchStrides& ref_strides,
			SketchStrides& query_strides,
			std::vector<Reference>& ref_sketches,
			std::vector<Reference>& query_sketches,
			const SketchSlice& sample_slice,
			const FlatRandom& flat_random,
			const std::vector<uint16_t>& ref_random_idx,
			const std::vector<uint16_t>& query_random_idx,
			const std::vector<size_t>& kmer_lengths,
			long long dist_rows,
			const bool self,
      const int cpu_threads);

		~DeviceMemory();

		std::vector<float> read_dists();

		uint64_t * ref_sketches() { return d_ref_sketches; }
		uint64_t * query_sketches() { return d_query_sketches; }
		float * random_table() { return d_random_table; }
		uint16_t * ref_random() { return d_ref_random; }
		uint16_t * query_random() { return d_query_random; }
		int * kmers() { return d_kmers; }
		float * dist_mat() { return d_dist_mat; }

	private:
		DeviceMemory ( const DeviceMemory & ) = delete;
    DeviceMemory ( DeviceMemory && ) = delete;

		size_t _n_dists;
		uint64_t * d_ref_sketches;
		uint64_t * d_query_sketches;
		float * d_random_table;
		uint16_t * d_ref_random;
		uint16_t * d_query_random;
		int * d_kmers;
		float * d_dist_mat;
};

// defined in sketch.cu
class GPUCountMin {
    public:
        GPUCountMin();
        ~GPUCountMin();

        unsigned int * get_table() { return _d_countmin_table; }

        void reset();

    private:
        // delete move and copy to avoid accidentally using them
        GPUCountMin ( const GPUCountMin & ) = delete;
        GPUCountMin ( GPUCountMin && ) = delete;

        unsigned int * _d_countmin_table;

        const unsigned int _table_width_bits;
        const uint64_t _mask;
        const uint32_t _table_width;
        const int _hash_per_hash;
        const int _table_rows;
        const size_t _table_cells;
};

class DeviceReads {
    public:
        DeviceReads(const SeqBuf& seq_in, const size_t n_threads);
        ~DeviceReads();

        char * read_ptr() { return d_reads; }
        size_t count() const { return n_reads; }
        size_t length() const { return read_length; }
        size_t stride() const { return read_stride; }

    private:
        // delete move and copy to avoid accidentally using them
        DeviceReads ( const DeviceReads & ) = delete;
        DeviceReads ( DeviceReads && ) = delete;

        char * d_reads;
        size_t n_reads;
        size_t read_length;
        size_t read_stride;
};

void copyNtHashTablesToDevice();

std::vector<uint64_t> get_signs(DeviceReads& reads,
                                GPUCountMin& countmin,
                                const int k,
                                const bool use_rc,
                                const uint16_t min_count,
                                const uint64_t binsize,
                                const uint64_t nbins);

#endif

