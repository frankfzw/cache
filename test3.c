/**
 * Cache simulator test case - Aliasing
 */

#include "cache.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define STAT_ASSERT(c, r, rm, w, wm) do {                       \
                assert(c->stat_data_read == (r));               \
                assert(c->stat_data_read_miss == (rm));         \
                assert(c->stat_data_write == (w));              \
                assert(c->stat_data_write_miss == (wm));        \
        } while (0)

#define TEST_SIMPLE_STAT() \
        if (type == AVDC_READ)                                  \
                STAT_ASSERT(cache, hits+misses, misses, 0, 0);   \
        else if (type == AVDC_WRITE)                            \
                STAT_ASSERT(cache, 0, 0, hits+misses, misses);  \
        else                                                    \
                abort()

static void
test_random(avdark_cache_t *cache, avdc_pa_t alias_offset, avdc_access_type_t type)
{
        int i;
        int hits = 0;
        int misses = 0;

        avdc_reset_statistics(cache);
        STAT_ASSERT(cache, 0, 0, 0, 0);

        /* Access all ways in the first set, we should get 1 miss per way */
        for (i = 0; i < cache->assoc; i++) {
                avdc_access(cache, alias_offset * i, type);
                misses++;
                TEST_SIMPLE_STAT();
        }

        /* Now, access all the ways again, we shouldn't get any misses */
        for (i = 0; i < cache->assoc; i++) {
                avdc_access(cache, alias_offset * i, type);
                hits++;
                TEST_SIMPLE_STAT();
        }

        /* Access 1 cache line that aliases into set 0 and replaces
         * a line that we just loaded */
        avdc_access(cache, alias_offset * cache->assoc, type);
        misses++;
        TEST_SIMPLE_STAT();

        //access again, it should hit
        
        avdc_access(cache, alias_offset * cache->assoc, type);
        hits ++;
        TEST_SIMPLE_STAT();
}

static void
test_fifo(avdark_cache_t *cache, avdc_pa_t alias_offset, avdc_access_type_t type)
{
        int i;
        int hits = 0;
        int misses = 0;

        avdc_reset_statistics(cache);
        STAT_ASSERT(cache, 0, 0, 0, 0);

        /* Access all ways in the first set, we should get 1 miss per way */
        for (i = 0; i < cache->assoc; i++) {
                avdc_access(cache, alias_offset * i, type);
                misses++;
                TEST_SIMPLE_STAT();
        }

        /* Now, access all the ways again, we shouldn't get any misses */
        for (i = 0; i < cache->assoc; i++) {
                avdc_access(cache, alias_offset * i, type);
                hits++;
                TEST_SIMPLE_STAT();
        }
        
        /* Now, access the first line again, this should be a hit and has the biggest count*/
        avdc_access(cache, 0, type);
        hits++;
        TEST_SIMPLE_STAT();

        /* Access 1 cache line that aliases into set 0 and replaces
         * a line that we just loaded */
        avdc_access(cache, alias_offset * cache->assoc, type);
        misses++;
        TEST_SIMPLE_STAT();

        
        /* Now, access the first line again, this should be a miss since it the first element of the cache set*/
        avdc_access(cache, 0, type);
        misses++;
        TEST_SIMPLE_STAT();
}

int
main(int argc, char *argv[])
{
        avdark_cache_t *cache;

	//test random
        cache = avdc_new(512, 64, 1, "RANDOM");
        assert(cache);
        avdc_print_info(cache);

        printf("Random [read]\n");
        test_random(cache, 512, AVDC_READ);
        printf("Random [write]\n");
        avdc_flush_cache(cache);
        test_random(cache, 512, AVDC_WRITE);


        avdc_resize(cache, 512, 128, 1);
        avdc_print_info(cache);

        printf("Random [read]\n");
        test_random(cache, 512, AVDC_READ);
        printf("Random [write]\n");
        avdc_flush_cache(cache);
        test_random(cache, 512, AVDC_WRITE);


        avdc_resize(cache, 256, 64, 1);
        avdc_print_info(cache);

        printf("Random [read]\n");
        test_random(cache, 256, AVDC_READ);
        printf("Random [write]\n");
        avdc_flush_cache(cache);
        test_random(cache, 256, AVDC_WRITE);


        printf("Switching to full assoc, RANDOM\n");

        avdc_resize(cache, 512, 64, 8);
        avdc_print_info(cache);

        printf("Random [read]\n");
        test_random(cache, 64, AVDC_READ);
        printf("Random [write]\n");
        avdc_flush_cache(cache);
        test_random(cache, 64, AVDC_WRITE);

 
        avdc_delete(cache);
        
        printf("%s random check done.\n", argv[0]);
        
        //test fifo
        //test random
        cache = avdc_new(512, 64, 1, "FIFO");
        assert(cache);
        avdc_print_info(cache);

        printf("FIFO [read]\n");
        test_fifo(cache, 512, AVDC_READ);
        printf("FIFO [write]\n");
        avdc_flush_cache(cache);
        test_fifo(cache, 512, AVDC_WRITE);


        avdc_resize(cache, 512, 128, 1);
        avdc_print_info(cache);

        printf("FIFO [read]\n");
        test_fifo(cache, 512, AVDC_READ);
        printf("FIFO [write]\n");
        avdc_flush_cache(cache);
        test_fifo(cache, 512, AVDC_WRITE);


        avdc_resize(cache, 256, 64, 1);
        avdc_print_info(cache);

        printf("FIFO [read]\n");
        test_fifo(cache, 256, AVDC_READ);
        printf("FIFO [write]\n");
        avdc_flush_cache(cache);
        test_fifo(cache, 256, AVDC_WRITE);


        printf("Switching to full assoc, FIFO\n");

        avdc_resize(cache, 512, 64, 8);
        avdc_print_info(cache);

        printf("FIFO [read]\n");
        test_fifo(cache, 64, AVDC_READ);
        printf("FIFO [write]\n");
        avdc_flush_cache(cache);
        test_fifo(cache, 64, AVDC_WRITE);

 
        avdc_delete(cache);

        printf("%s FIFO check done.\n", argv[0]);
        return 0;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * c-file-style: "linux"
 * compile-command: "make -k -C ../../"
 * End:
 */
