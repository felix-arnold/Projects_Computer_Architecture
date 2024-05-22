#include "cache.hpp"

// Constructor for Cache class
Cache::Cache(cache_config_t config) {
    // Initialize cache parameters from configuration
    disabled = config.disabled;
    prefetcher_disabled = config.prefetcher_disabled;
    strided_prefetch_disabled = config.strided_prefetch_disabled;
    total_size_c = config.c;
    block_size_b = config.b;
    way_number_s = config.s;
    replace_policy = config.replace_policy;
    prefetch_insert_policy = config.prefetch_insert_policy;
    write_strat = config.write_strat;

    set_number = total_size_c - block_size_b - way_number_s;
    lru_counter = 1 << (total_size_c - block_size_b + 1);
    initialize_cache_memory_size();
}

// Initialize cache memory size
void Cache::initialize_cache_memory_size() {
    for (uint64_t i = 0; i < ((uint64_t)1 << set_number); i++) {
        cache_memory.emplace(i, std::vector<cache_block_t>{});
        for (uint64_t j = 0; j < ((uint64_t)1 << way_number_s); j++)
            cache_memory[i].emplace_back();
    }
}

// Compute tag from address
uint64_t Cache::compute_tag(uint64_t addr) {
    return addr >> (set_number + block_size_b);
}

// Compute index from address
uint64_t Cache::compute_index(uint64_t addr) {
    return (set_number) ? (addr << (64 - set_number - block_size_b)) >> (64 - set_number) : 0;
}

// Recompute address from tag and index
uint64_t Cache::compute_address(uint64_t tag, uint64_t index) {
    return (tag << (set_number + block_size_b)) | (index << block_size_b);
}

// Check if the data pointed by the input address is stored in cache
access_result_t Cache::check_address(char rw, uint64_t addr, sim_stats_t* stats, bool is_prefetching) {

    uint64_t tag = compute_tag(addr);
    uint64_t index = compute_index(addr);
    #ifdef DEBUG
        if (write_strat == WRITE_STRAT_WBWA)
            printf("L1 ");
        else
            printf("L2 ");
        printf("decomposed address 0x%" PRIx64 " -> Tag: 0x%" PRIx64 " and Index: 0x%" PRIx64 "\n", addr, tag, index);
    #endif

    // Find block in cache memory
    auto block = std::find_if(cache_memory[index].begin(), cache_memory[index].end(), [&](const cache_block_t& block) {
        return block.tag == tag;
    });
    
    // Cache hit
    if (!disabled && block != cache_memory[index].end() && block->valid_bit == true) {   
        if (!is_prefetching) {

            // Update LRU or MRU information     
            if (replace_policy == REPLACE_POLICY_LRU)
                block->lru_timestamp = lru_counter++;
            else {
                for (cache_block_t& blk : cache_memory[index])
                    blk.mru_bit = false;
                block->mru_bit = true;
                block->mru_counter++;
            }
    
            // Set dirty bit for write operation
            if (rw == WRITE)
                block->dirty_bit = true;

            if (write_strat == WRITE_STRAT_WBWA) {
                #ifdef DEBUG
                    printf("L1 hit\n");
                    printf("In L1, moving Tag: 0x%" PRIx64 " and Index: 0x%" PRIx64 " to MRU position", tag, index);
                    if (block->dirty_bit)
                        printf(" and setting dirty bit\n");
                    else
                        printf("\n");
                #endif

                stats->accesses_l1++;
                stats->hits_l1++;
            }
            else {
                #ifdef DEBUG
                    if (rw==WRITE)
                        printf("L2 found block in cache on write\n");
                    else
                        printf("L2 read hit\n");
                    printf("In L2, moving Tag: 0x%" PRIx64 " and Index: 0x%" PRIx64 " to MRU position\n", tag, index);
                #endif

                if (rw == READ) {
                    stats->reads_l2++;
                    stats->read_hits_l2++;
                }
                else
                    stats->writes_l2++;
            }
        }
        return HIT;
    }

    // Cache miss
    if (write_strat == WRITE_STRAT_WBWA) {
        #ifdef DEBUG
            printf("L1 miss\n");
        #endif

        stats->accesses_l1++;
        stats->misses_l1++;
    }
    else if (!is_prefetching) {
        #ifdef DEBUG
        if (disabled)
            printf("L2 is disabled, treating this as an L2 read miss\n");
        else
            printf("L2 read miss\n");
        #endif

        if (rw == READ) {
            stats->reads_l2++;
            stats->read_misses_l2++;
        }
        else 
            stats->writes_l2++;
    }
    
    return MISS;
}

// Add block to cache
std::optional<uint64_t> Cache::add_block(char rw, uint64_t addr, sim_stats_t* stats, bool is_prefetching) {
    if (disabled)
        return std::nullopt;

    if (replace_policy == REPLACE_POLICY_LRU)
        return lru_add_block(rw, addr, stats, is_prefetching);
    else
        return lfu_add_block(rw, addr, stats, is_prefetching);
}

// Add block to cache using LRU replacement policy
std::optional<uint64_t> Cache::lru_add_block(char rw, uint64_t addr, sim_stats_t* stats, bool is_prefetching) {
    uint64_t index = compute_index(addr);
    uint64_t evict_i = UINT64_MAX;
    uint64_t min_timestamp = UINT64_MAX;

    // Find least recently used block
    for (uint64_t i = 0; i < ((uint64_t)1 << way_number_s); i++) {
        if (!cache_memory[index].at(i).valid_bit) {
            evict_i = i;
            break;
        }
        if (cache_memory[index].at(i).lru_timestamp < min_timestamp) {
            min_timestamp = cache_memory[index].at(i).lru_timestamp;
            evict_i = i;
        }
    }
    cache_block_t previous_block = cache_memory[index].at(evict_i);

    // Replace block with new block
    cache_memory[index].at(evict_i) = {
        .tag = compute_tag(addr),
        .valid_bit = true,
        .dirty_bit = (rw == WRITE && write_strat == WRITE_STRAT_WBWA) ? true : false,
        .lru_timestamp = (prefetch_insert_policy == INSERT_POLICY_MIP || !is_prefetching) ? lru_counter : std::min(min_timestamp-1, lru_counter)
    };
    lru_counter++;
    
    #if DEBUG
        if (!is_prefetching) {
            printf("Evict from %s: ", (write_strat==WRITE_STRAT_WBWA ? "L1" : "L2"));
            if (previous_block.valid_bit)
                printf("block with valid=1, dirty=%d, tag 0x%" PRIx64 " and index=0x%" PRIx64 "\n", previous_block.dirty_bit, previous_block.tag, index);
            else
                printf("block with valid=0 and index=0x%" PRIx64 "\n", index);
        }
    #endif
    
    // Return evicted block's address if it's dirty
    if (write_strat==WRITE_STRAT_WBWA && previous_block.dirty_bit)
        return compute_address(previous_block.tag, index);

    return std::nullopt;
}

// Add block to cache using LFU replacement policy
std::optional<uint64_t> Cache::lfu_add_block(char rw, uint64_t addr, sim_stats_t *stats, bool is_prefetching) {
    uint64_t index = compute_index(addr);
    uint64_t evict_i = UINT64_MAX;
    uint64_t min_counter = UINT64_MAX;

    // Find least frequently used block
    for (uint64_t i = 0; i < ((uint64_t)1 << way_number_s); i++) {
        if (!cache_memory[index].at(i).valid_bit) {
            evict_i = i;
            break;
        }
        if (!cache_memory[index].at(i).mru_bit) {
            if (cache_memory[index].at(i).mru_counter < min_counter) {
                min_counter = cache_memory[index].at(i).mru_counter;
                evict_i = i;
            }
            else if (cache_memory[index].at(i).mru_counter == min_counter) {
                if (cache_memory[index].at(i).tag < cache_memory[index].at(evict_i).tag)
                    evict_i = i;
            }
        }
        else if (prefetch_insert_policy == INSERT_POLICY_MIP || !is_prefetching)
            cache_memory[index].at(i).mru_bit = false;
    }

    cache_block_t previous_block = cache_memory[index].at(evict_i);

    // Replace block with new block
    cache_memory[index].at(evict_i) = {
        .tag = compute_tag(addr),
        .valid_bit = true,
        .dirty_bit = (rw == WRITE && write_strat == WRITE_STRAT_WBWA) ? true : false,
        .mru_counter = (is_prefetching) ? (uint64_t) 0 : (uint64_t) 1,
        .mru_bit = (is_prefetching && prefetch_insert_policy == INSERT_POLICY_LIP) ? false : true
    };

    #if DEBUG
        if (!is_prefetching) {
            printf("Evict from %s: ", (write_strat==WRITE_STRAT_WBWA ? "L1" : "L2"));
            if (previous_block.valid_bit)
                printf("block with valid=1, dirty=%d, tag 0x%" PRIx64 " and index=0x%" PRIx64 "\n", previous_block.dirty_bit, previous_block.tag, index);
            else
                printf("block with valid=0 and index=0x%" PRIx64 "\n", index);
        }
    #endif

    // Return evicted block's address if it's dirty
    if (write_strat==WRITE_STRAT_WBWA && previous_block.dirty_bit)
        return compute_address(previous_block.tag, index);

    return std::nullopt;
}

// Prefetch block into cache
void Cache::prefetch(uint64_t addr, sim_stats_t *stats) {

    if (disabled || prefetcher_disabled)
        return;

    #ifdef DEBUG
        printf("Prefetch block with address 0x%" PRIx64 " from memrory to L2\n", addr);
    #endif

    if (strided_prefetch_disabled)
        one_plus_prefetch(addr, stats);
    else 
        strided_prefetch(addr, stats);
}

// Perform one-plus prefetch
void Cache::one_plus_prefetch(uint64_t addr, sim_stats_t *stats) {
    uint64_t prefetch_addr = ((addr >> block_size_b) + 1) << block_size_b;

    // Check if prefetch address is not in cache
    if (check_address(WRITE, prefetch_addr, stats, true) == MISS) {
        stats->prefetches_l2++;
        add_block(READ, prefetch_addr, stats, true);
    }
}

// Perform strided prefetch
void Cache::strided_prefetch(uint64_t addr, sim_stats_t *stats) {
    static uint64_t previous_miss = 0x0;
    uint64_t prefetch_addr = ((addr >> block_size_b) + ((addr >> block_size_b) - (previous_miss >> block_size_b))) << block_size_b;

    // Check if prefetch address is not in cache
    if (check_address(WRITE, prefetch_addr, stats, true) == MISS) {
        stats->prefetches_l2++;
        add_block(READ, prefetch_addr, stats, true);
    }
    previous_miss = addr;
}

// Get the number of sets in the cache
uint64_t Cache::get_set_number() {
    return set_number;
}

// Get the number of ways in the cache
uint64_t Cache::get_way_number_s() {
    return way_number_s;
}

// Check if cache is disabled
bool Cache::is_disabled() {
    return disabled;
}
