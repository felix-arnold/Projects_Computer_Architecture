#include "cache.h"

#include <assert.h>
#include <stdio.h>

Cache::Cache(nodeID_t id) {
    this->id = id;
    entries.clear();
}

Cache::~Cache() {
    for (auto entry : entries) {
        delete entry.second;
    }
}

void Cache::tick() {
    if (!my_node->done) debug_printf("\tTick for cache %lu\n", id);
    if (cpu_in) {
        cache_entry_t* entry = get_entry(cpu_in->block);
        assert(entry);
        entry->agent->process_proc_request(cpu_in);
        delete cpu_in;
        cpu_in = NULL;
    }

    if (ntwk_in) {
        cache_entry_t* entry = get_entry(ntwk_in->block);
        assert(entry);
        entry->agent->process_ntwk_request(ntwk_in);
        delete ntwk_in;
        ntwk_in = NULL;
    }
}

void Cache::tock() {
    if (cpu_in_next) {
        cpu_in = cpu_in_next;
        cpu_in_next = NULL;
    }

    if (ntwk_in_next) {
        ntwk_in = ntwk_in_next;
        ntwk_in_next = NULL;
    }
}

cache_entry_t* Cache::get_entry(blockID_t block) {
    blockID_t tag = block;
    auto it = entries.find(tag);
    cache_entry_t* entry;
    if (it == entries.end()) {
        entry = new cache_entry_t(this->id, this, tag, settings.protocol);
        entries.insert(std::pair<blockID_t, cache_entry_t*>(tag, entry));
    } else {
        entry = it->second;
    }
    return entry;
}
