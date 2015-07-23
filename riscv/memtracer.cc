// See LICENSE for license details.

#include "memtracer.h"
#include "cachesim.h"
#include "common.h"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <queue>

void memtracer_list_t::add_cache(cache_sim_t *cache) {
  std::vector<cache_sim_t*>::iterator it;
  std::queue<cache_sim_t*> cache_queue;
  cache_queue.push(cache);
  while(!cache_queue.empty()) {
    cache = cache_queue.front();
    cache_queue.pop();
    if(cache == NULL) continue;

    // Check if we already pushed this cache
    it = find (cache_buf.begin(), cache_buf.end(), cache);
    if(it == cache_buf.end())
      cache_buf.push_back(cache);

    // Recurse on the miss handlers
    int i;
    for(i = 0; i < cache->get_n_miss_handlers(); i++) {
      cache_queue.push(cache->get_miss_handler(i));
    }
  }
}

void memtracer_list_t::get_all_caches() {
  cache_buf.clear();

  for(uint32_t i = 0; i < list.size(); i++) {
    memtracer_t *elt = list[i];

    if(elt->is_list()) {
      memtracer_list_t *sublist = (memtracer_list_t*) elt;
      sublist->get_all_caches();
      for(uint32_t j = 0; j < sublist->cache_buf.size(); j++) {
        add_cache(sublist->cache_buf[j]);
      }
    } else {
      add_cache(elt->get_cache());
    }
  }

}

void memtracer_list_t::update_stats(cache_info_t* cache_info) {
  get_all_caches();
  uint32_t ind = 0;
  void *tmp;

  cache_info->n_caches = cache_buf.size();

  for(uint32_t i = 0; i < cache_buf.size(); i++) {
    cache_stats_t* stats = &cache_info->stats[i];
    cache_buf[i]->write_stats(stats);
  }
}
