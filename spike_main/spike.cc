// See LICENSE for license details.

#include "sim.h"
#include "htif.h"
#include "cachesim.h"
#include "extension.h"
#include "tagstats.h"
#include <dlfcn.h>
#include <fesvr/option_parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <vector>
#include <string>
#include <memory>

#include "../riscv/tagpolicy.h"

static void help()
{
  fprintf(stderr, "usage: spike [host options] <target program> [target options]\n");
  fprintf(stderr, "Host Options:\n");
  fprintf(stderr, "  -p <n>             Simulate <n> processors\n");
  fprintf(stderr, "  -m <n>             Provide <n> MB of target memory\n");
  fprintf(stderr, "  -d                 Interactive debug mode\n");
  fprintf(stderr, "  -g                 Track histogram of PCs\n");
  fprintf(stderr, "  -t                 Allow for tracing memory writes\n");
  fprintf(stderr, "  -h                 Print this help message\n");
  fprintf(stderr, "  -s                 Track tag statistics\n");
  fprintf(stderr, "  -k                 Turn on dynamic memory tracing\n");
  fprintf(stderr, "  --ic=<S>:<W>:<B>   Instantiate a cache model with S sets,\n");
  fprintf(stderr, "  --dc=<S>:<W>:<B>     W ways, and B-byte blocks (with S and\n");
  fprintf(stderr, "  --l2=<S>:<W>:<B>     B both powers of 2).\n");
  fprintf(stderr, "  --tc=<S>:<W>:<B>   Tag cache\n");
  fprintf(stderr, "  --extension=<name> Specify RoCC Extension\n");
  fprintf(stderr, "  --extlib=<name>    Shared library to load\n");
  exit(1);
}

int main(int argc, char** argv)
{


    // todo move this to tagpolicy.h
    printf("tag policies enabled:\n");
    printf("\tTAG_POLICY_MATCH_CALLRET: %s\n",
#if defined(TAG_POLICY_MATCH_CALLRET) 
           "enabled");
#else
           "disabled");
#endif
    printf("\tTAG_POLICY_NO_RETURN_COPY: %s\n", 
#if defined(TAG_POLICY_NO_RETURN_COPY) 
           "enabled");
#else
           "disabled");
#endif
    printf("\tTAG_POLICY_FP: %s\n", 
#if defined(TAG_POLICY_FP) 
           "enabled");
#else
           "disabled");
#endif
    printf("\tTAG_POLICY_NO_PARTIAL_COPY: %s\n",
#if defined(TAG_POLICY_NO_PARTIAL_COPY)
           "enabled");
#else
           "disabled");
#endif




  bool debug = false;
  bool histogram = false;
  bool use_watch_loc = false;
  bool use_tracker = false;
  size_t nprocs = 1;
  size_t mem_mb = 0;
  bool tag_stats = false;
  std::unique_ptr<icache_sim_t> ic;
  std::unique_ptr<dcache_sim_t> dc;
  std::unique_ptr<cache_sim_t> l2;
  std::unique_ptr<cache_sim_t> tc;
  std::function<extension_t*()> extension;
  std::unique_ptr<tag_memtracer_t> tagtracer;

  option_parser_t parser;
  parser.help(&help);
  parser.option('h', 0, 0, [&](const char* s){help();});
  parser.option('d', 0, 0, [&](const char* s){debug = true;});
  parser.option('g', 0, 0, [&](const char* s){histogram = true;});
  parser.option('s', 0, 0, [&](const char* s){tag_stats = true;});
  parser.option('p', 0, 1, [&](const char* s){nprocs = atoi(s);});
  parser.option('m', 0, 1, [&](const char* s){mem_mb = atoi(s);});
  parser.option('t', 0, 0, [&](const char* s){use_watch_loc = true;});
  parser.option('k', 0, 0, [&](const char* s){use_tracker = true;});
  parser.option(0, "ic", 1, [&](const char* s){ic.reset(new icache_sim_t(s));});
  parser.option(0, "dc", 1, [&](const char* s){dc.reset(new dcache_sim_t(s));});
  parser.option(0, "l2", 1, [&](const char* s){l2.reset(cache_sim_t::construct(s, "L2$"));});
  parser.option(0, "tc", 1, [&](const char* s){tc.reset(cache_sim_t::construct(s, "TC$")); tc->set_tag_mode(true); });
  parser.option(0, "extension", 1, [&](const char* s){extension = find_extension(s);});
  parser.option(0, "extlib", 1, [&](const char *s){
    void *lib = dlopen(s, RTLD_NOW | RTLD_GLOBAL);
    if (lib == NULL) {
      fprintf(stderr, "Unable to load extlib '%s': %s\n", s, dlerror());
      exit(-1);
    }
  });

  auto argv1 = parser.parse(argv);
  if (!*argv1)
    help();
  std::vector<std::string> htif_args(argv1, (const char*const*)argv + argc);
  sim_t s(nprocs, mem_mb, htif_args);

  if(tag_stats) {
    tagtracer.reset(new tag_memtracer_t(s.get_tagmem(), s.get_memsz() / MEM_TO_TAG_RATIO));
  }

  if (l2 && tc) {
    size_t sets = tc->get_sets();
    size_t ways = tc->get_ways();
    size_t linesz = tc->get_linesz();
    l2->add_miss_handler(&*tc);
    for(int i = 0; i < 10; i++) {
      sets *= 2;
      cache_sim_t *bigger_tc = new cache_sim_t(sets, ways, linesz, "TC$");
      bigger_tc->set_tag_mode(true);
      l2->add_miss_handler(bigger_tc);
    }
  }
  if (ic && l2) ic->add_miss_handler(&*l2);
  if (dc && l2) dc->add_miss_handler(&*l2);
  if (ic) s.get_debug_mmu()->register_memtracer(&*ic);
  if (dc) s.get_debug_mmu()->register_memtracer(&*dc);
  if (tagtracer) s.get_debug_mmu()->register_memtracer(&*tagtracer);
  for (size_t i = 0; i < nprocs; i++)
  {
    if (use_tracker) s.get_core(i)->init_tracker();
    if (ic) s.get_core(i)->get_mmu()->register_memtracer(&*ic);
    if (dc) s.get_core(i)->get_mmu()->register_memtracer(&*dc);
    if (tagtracer) s.get_core(i)->get_mmu()->register_memtracer(&*tagtracer);
    if (extension) s.get_core(i)->register_extension(extension());
    if (use_watch_loc) {
        // Set up memory tracing for each core.
        state_t* st = s.get_core(i)->get_state();
        watch_loc* wl = new watch_loc(st);
        s.get_core(i)->get_mmu()->set_watch_loc(wl);
    }
  }

  s.set_debug(debug);
  s.set_histogram(histogram);
  return s.run();
}
