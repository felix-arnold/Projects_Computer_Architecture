#include "cachesim.hpp"

// Declare two shared pointers to Cache objects, one for L1 cache and one for L2 cache
std::shared_ptr<Cache> l1_cache ;
std::shared_ptr<Cache> l2_cache; 

/**
 * Subroutine for initializing the cache simulator. You many add and initialize any global or heap
 * variables as needed.
 */
void sim_setup(sim_config_t *config) {
    // Create the Cache objects using the configuration parameters and assign them to the cache pointers
    l1_cache = std::make_shared<Cache>(config->l1_config);
    l2_cache = std::make_shared<Cache>(config->l2_config);
}

/**
 * Subroutine that simulates the cache one trace event at a time.
 */
void sim_access(char rw, uint64_t addr, sim_stats_t* stats) {

    #ifdef DEBUG
        static uint64_t time = 0;
        printf("Time: %" PRIu64 ". Address: 0x%" PRIx64 ". Read/Write: %c\n", time++, addr, rw);
    #endif

    // Increment the read or write statistics based on the rw parameter
    if (rw == READ)
        stats->reads++;
    else
        stats->writes++;

    // Check if the address is present in the L1 cache
    if (l1_cache->check_address(rw, addr, stats, false) == MISS) {
        // If not, check if the address is present in the L2 cache
        if (l2_cache->check_address(READ, addr, stats, false) == MISS) {
            // If not, add the block containing the address to the L2 cache
            l2_cache->add_block(READ, addr, stats, false);
            // Perform prefetching on the L2 cache
            l2_cache->prefetch(addr, stats);
        } 
        // Add the block containing the address to the L1 cache and return the evicted block if any
        auto dirty_block = l1_cache->add_block(rw, addr, stats, false);
        // If there is an evicted block and it is dirty, write it back to the L2 cache if L2 contains it
        if (dirty_block.has_value()) {
            #if DEBUG
                printf("Writing back dirty block with address 0x%" PRIx64 " to L2\n", dirty_block.value());
            #endif

            access_result_t dirty_check = l2_cache->check_address(WRITE, dirty_block.value(), stats, false);

            #ifdef DEBUG
                if (dirty_check == MISS) {
                    printf("L2 did not find block in cache on write, writing through to memory anyway\n");
                }
            #endif
        }
    }
    #ifdef DEBUG
        printf("\n");
    #endif
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 */
void sim_finish(sim_stats_t *stats) {
    // Calculate the read hit ratio, read miss ratio, hit time and average access time for the L2 cache
    // If L2 is disabled, the hit time is set to 0
    stats->read_hit_ratio_l2 = (double) stats->read_hits_l2 / (double) stats->reads_l2;
    stats->read_miss_ratio_l2 = (double) stats->read_misses_l2 / (double) stats->reads_l2;
    double hit_time_l2 = (l2_cache->is_disabled()) ? 0.0 : L2_HIT_K3 + (L2_HIT_K4 * l2_cache->get_set_number()) + L2_HIT_K5 * (std::max((uint64_t) 3, l2_cache->get_way_number_s()) - 3);
    stats->avg_access_time_l2 = hit_time_l2 + stats->read_miss_ratio_l2 * DRAM_ACCESS_TIME;

    // Calculate the read hit ratio, read miss ratio, hit time and average access time for the L1 cache
    stats->hit_ratio_l1 = (double) stats->hits_l1 / (double) stats->accesses_l1;
    stats->miss_ratio_l1 = (double) stats->misses_l1 / (double) stats->accesses_l1;
    double hit_time_l1 = L1_HIT_K0 + (L1_HIT_K1 * l1_cache->get_set_number()) + L1_HIT_K2 * (std::max((uint64_t) 3, l1_cache->get_way_number_s()) - 3);
    stats->avg_access_time_l1 = hit_time_l1 + stats->miss_ratio_l1 * stats->avg_access_time_l2;
}
