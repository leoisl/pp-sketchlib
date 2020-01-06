/*
 *
 * seqio.cpp
 * Sequence reader and buffer class
 *
 */

#include "seqio.hpp"
#include <stdint.h>
KSEQ_INIT(gzFile, gzread)

// C/C++/C++11/C++17 headers
#include <inttypes.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <iterator>
#include <algorithm>
#include <utility>

const unsigned char RCMAP[256] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
'T', 66,'G', 68, 69, 70,'C',
 72, 73, 74, 75, 76, 77, 78,
 79, 80, 81,     82, 83,'A',
 85, 86, 87,     88, 89, 90,
 91, 92, 93,     94, 95, 96,
't', 98,'g',100,101,102,'c',
104,105,106,107,108,109,110,
111,112,113,    114,115,'a',
117,118,119,    120,121,122,
123,124,125,126,127,
128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};


// code from https://stackoverflow.com/questions/735204/convert-a-string-in-c-to-upper-case
char ascii_toupper_char(char c) {
    return ('a' <= c && c <= 'z') ? c^0x20 : c;    // ^ autovectorizes to PXOR: runs on more ports than paddb
}

SeqBuf::SeqBuf(const std::vector<std::string>& filenames, const size_t kmer_len)
{
    /* 
    *   Reads entire sequence to memory
    *   May be faster as hashing at multiple k-mers?
    */
    _reads = false;
    for (auto name_it = filenames.begin(); name_it != filenames.end(); name_it++)
    {
        // from kseq.h
        gzFile fp = gzopen(name_it->c_str(), "r");
        kseq_t *seq = kseq_init(fp);
        int l;
        while ((l = kseq_read(seq)) >= 0) 
        {
            if (strlen(seq->seq.s) >= kmer_len)
            {
                sequence.push_back(seq->seq.s);
                for (char & c : sequence.back()) 
                {
                    c = ascii_toupper_char(c);
                }

                rc_sequence.push_back(sequence.back());
                for (char & c : rc_sequence.back())
                {
                    c = RCMAP[(int)c];
                }
                std::reverse(rc_sequence.back().begin(), rc_sequence.back().end());             
            }
            
            // Presence of any quality scores - assume reads as input
            if (seq->qual.l)
            {
                _reads = true;
            }
        }
        
        // If put back into object, move this to destructor below
        kseq_destroy(seq);
        gzclose(fp);
    }
    this->reset();
}

void SeqBuf::reset()
{
    /* 
    *   Returns to start of sequences
    */
    if (sequence.size() > 0)
    {
        current_base = 0;
        current_seq = 0;
    }
    _end = false;
}

void SeqBuf::move_next(const size_t kmer_len)
{
    /* 
    *   Moves along to next character in sequence and reverse complement
    *   Loops around to next sequence if end reached
    *   Keeps track of base before k-mer length 
    */
    if (!_end)
    {
        current_base++;
        if (current_base + kmer_len > sequence[current_seq].size())
        {
            current_seq++;
            current_base = 0;
            if (current_seq == sequence.size())
            {
                _end = true;
            }
        }
    }
}
