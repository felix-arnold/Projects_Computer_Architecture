#ifndef CACHE_HPP
#define CACHE_HPP

#include <cstdint>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "cache_config.hpp"

typedef enum access_result {
    HIT,
    MISS
} access_result_t;

typedef struct cache_block {
    uint64_t tag = -1;
    bool valid_bit = false;
    bool dirty_bit = false;
    
    uint64_t lru_timestamp = UINT64_MAX;

    uint64_t mru_counter = 0;
    bool mru_bit = false;
} cache_block_t;

class Cache {
    public:
        Cache(cache_config_t config);
        ~Cache() = default;

        access_result_t check_address(char rw, uint64_t addr, sim_stats_t* stats, bool is_prefetching);
        std::optional<uint64_t> add_block(char rw, uint64_t addr, sim_stats_t* stats, bool is_prefetching);
        void prefetch(uint64_t addr, sim_stats_t* stats);

        uint64_t get_set_number();
        uint64_t get_way_number_s();

        bool is_disabled();

    private:
        bool disabled;
        bool prefetcher_disabled;
        bool strided_prefetch_disabled;
        uint64_t total_size_c;
        uint64_t block_size_b;
        uint64_t way_number_s;
        replace_policy_t replace_policy;
        insert_policy_t prefetch_insert_policy;
        write_strat_t write_strat;

        uint64_t set_number;
        std::unordered_map<uint64_t, std::vector<cache_block_t>> cache_memory;

        uint64_t lru_counter;

        void initialize_cache_memory_size();
        uint64_t compute_tag(uint64_t addr);
        uint64_t compute_index(uint64_t addr);
        uint64_t compute_address(uint64_t tag, uint64_t index);

        std::optional<uint64_t> lru_add_block(char rw, uint64_t addr, sim_stats_t* stats, bool is_prefetching);
        std::optional<uint64_t> lfu_add_block(char rw, uint64_t addr, sim_stats_t* stats, bool is_prefetching);

        void one_plus_prefetch(uint64_t addr, sim_stats_t* stats);
        void strided_prefetch(uint64_t addr, sim_stats_t *stats);
};

#endif