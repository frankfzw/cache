/**
 * Cache simulation using a functional system simulator.
 */

#include "cache.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <time.h>


#define AVDC_MALLOC(nelems, type) malloc(nelems * sizeof(type))
#define AVDC_FREE(p) free(p)

/**
 * Cache block information.
 *
 * HINT: You will probably need to change this structure
 */
struct avdc_cache_line {
	unsigned long long count;
	avdc_tag_t tag;
        int        valid;
};

/**
 * Extract the cache line tag from a physical address.
 *
 * You probably don't want to change this function, instead you may
 * want to change how the tag_shift field is calculated in
 * avdc_resize().
 */
static inline avdc_pa_t
tag_from_pa(avdark_cache_t *self, avdc_pa_t pa)
{
        return pa >> self->tag_shift;
}

/**
 * Calculate the cache line index from a physical address.
 *
 * Feel free to experiment and change this function
 */
static inline int
index_from_pa(avdark_cache_t *self, avdc_pa_t pa)
{
        return (pa >> self->block_size_log2) & (self->number_of_sets - 1);
}

/**
 * Computes the log2 of a 32 bit integer value. Used in dc_init
 *
 * Do NOT modify!
 */
static int
log2_int32(uint32_t value)
{
        int i;

        for (i = 0; i < 32; i++) {
                value >>= 1;
                if (value == 0)
                        break;
        }
        return i;
}

/**
 * Check if a number is a power of 2. Used for cache parameter sanity
 * checks.
 *
 * Do NOT modify!
 */
static int
is_power_of_two(uint64_t val)
{
        return ((((val)&(val-1)) == 0) && (val > 0));
}

void
avdc_dbg_log(avdark_cache_t *self, const char *msg, ...)
{
        va_list ap;
        
        if (self->dbg) {
                const char *name = self->dbg_name ? self->dbg_name : "AVDC";
                fprintf(stderr, "[%s] dbg: ", name);
                va_start(ap, msg);
                vfprintf(stderr, msg, ap);
                va_end(ap);
        }
}


void
avdc_access(avdark_cache_t *self, avdc_pa_t pa, avdc_access_type_t type)
{
        /* HINT: You will need to update this function */
        avdc_tag_t tag = tag_from_pa(self, pa);
        int index = index_from_pa(self, pa);
        int hit = 0;
        
        int added = 0;
		
		self->maxCount ++;
		
		int kickIndex = -1;
		if(self->replacement == LRU)
		{	
			//LRU 
			unsigned long long minCount = self->maxCount;
		    
			int i = 0;
			for (; i < self->assoc; i ++)
			{
				int temp = index * self->assoc + i;
				hit = self->lines[temp].valid && self->lines[temp].tag == tag;
			
				if (hit || (self->lines[temp].count == 0)) {
					self->lines[temp].valid = 1;
					self->lines[temp].tag = tag;
					self->lines[temp].count = self->maxCount;
					added = 1;
					break;
				}


			}
		
			if (!hit && !added) {
				for (i = 0; i < self->assoc; i ++)
				{
					int temp = index * self->assoc + i;
					//avdc_dbg_log(self, "kick: idx: %d, count: %llu\n", temp, self->lines[temp].count);
					if (self->lines[temp].count < minCount)
					{
						minCount = self->lines[temp].count;
						kickIndex = temp;
					}
				}
				self->lines[kickIndex].valid = 1;
				self->lines[kickIndex].tag = tag;
				self->lines[kickIndex].count = self->maxCount;
			}
		
		}
		else if (self->replacement == RANDOM)
		{
			//RANDOM
			
			int i = 0;
			for (; i < self->assoc; i ++)
			{
				int temp = index * self->assoc + i;
				hit = self->lines[temp].valid && self->lines[temp].tag == tag;
			
				if (hit || (self->lines[temp].count == 0)) {
					self->lines[temp].valid = 1;
					self->lines[temp].tag = tag;
					self->lines[temp].count = self->maxCount;
					added = 1;
					break;
				}
			}
			
			if (!hit && !added)
			{
				kickIndex = rand() % self->assoc + index * self->assoc;
				self->lines[kickIndex].valid = 1;
				self->lines[kickIndex].tag = tag;
				self->lines[kickIndex].count = self->maxCount;
			}
		}
		else if (self->replacement == FIFO)
		{
			//FIFO
			int i = 0;
			for (; i < self->assoc; i ++)
			{
				int temp = index * self->assoc + i;
				hit = self->lines[temp].valid && self->lines[temp].tag == tag;
			
				if (hit || (self->lines[temp].count == 0)) {
					self->lines[temp].valid = 1;
					self->lines[temp].tag = tag;
					self->lines[temp].count = self->maxCount;
					added = 1;
					break;
				}
			}
			
			if (!hit && !added)
			{
				kickIndex = self->first[index];
				self->first[index] = (kickIndex + 1) % self->assoc;
				kickIndex = index * self->assoc + kickIndex;
				self->lines[kickIndex].valid = 1;
				self->lines[kickIndex].tag = tag;
				self->lines[kickIndex].count = self->maxCount;
			}
		}
		else
		{
			avdc_dbg_log(self, "impossible replacement policy: %d\n", self->replacement);
		}
		
        switch (type) {
        case AVDC_READ: /* Read accesses */
                avdc_dbg_log(self, "read: pa: 0x%.16lx, tag: 0x%.16lx, index: %d, hit: %d, pos: %d , count: %llu\n",
                             (unsigned long)pa, (unsigned long)tag, index, hit, kickIndex, self->maxCount);
                self->stat_data_read += 1;
                if (!hit)
                        self->stat_data_read_miss += 1;
                break;

        case AVDC_WRITE: /* Write accesses */
                avdc_dbg_log(self, "write: pa: 0x%.16lx, tag: 0x%.16lx, index: %d, hit: %d, pos: %d, count: %llu, write count: %d\n",
                             (unsigned long)pa, (unsigned long)tag, index, hit, kickIndex, self->maxCount, self->stat_data_write);
                self->stat_data_write += 1;
                if (!hit)
                        self->stat_data_write_miss += 1;
                break;
        }
}

void
avdc_flush_cache(avdark_cache_t *self)
{
	int i;
        /* HINT: You will need to update this function */
        for (i = 0; i < self->number_of_sets; i++) {
        	int j = 0;
        	for (; j < self->assoc; j ++)
        	{
        		int temp = i * self->assoc + j;
                self->lines[temp].valid = 0;
                self->lines[temp].tag = 0;

				//added: flush count
				self->lines[temp].count = 0;
			}
        }
        
	self->maxCount = 0;
	memset(self->first, 0, sizeof(self->first));
}


int
avdc_resize(avdark_cache_t *self,
            avdc_size_t size, avdc_block_size_t block_size, avdc_assoc_t assoc)
{
        /* HINT: This function precomputes some common values and
         * allocates the self->lines array. You will need to update
         * this to reflect any changes to how this array is supposed
         * to be allocated.
         */

        /* Verify that the parameters are sane */
        if (!is_power_of_two(size) ||
            !is_power_of_two(block_size) ||
            !is_power_of_two(assoc)) {
                fprintf(stderr, "size, block-size and assoc all have to be powers of two and > zero\n");
                return 0;
        }

		avdc_dbg_log(self, "avdc resize: size %ld, block_szie %ld, assoc %d\n", (unsigned long)size, (unsigned long)block_size, (int)assoc);		
		
        /* Update the stored parameters */
        self->size = size;
        self->block_size = block_size;
        self->assoc = assoc;

        /* Cache some common values */
        self->number_of_sets = (self->size / self->block_size) / self->assoc;
        self->block_size_log2 = log2_int32(self->block_size);
        self->tag_shift = self->block_size_log2 + log2_int32(self->number_of_sets);


		//init the max accessing count to 0
		self->maxCount = 0;
		
		//init the first element queue
		self->first = (int *)malloc(self->number_of_sets * sizeof(int));
		memset(self->first, 0, sizeof(self->first));

        /* (Re-)Allocate space for the tags array */
        if (self->lines)
                AVDC_FREE(self->lines);
        /* HINT: If you change this, you may have to update
         * avdc_delete() to reflect changes to how thie self->lines
         * array is allocated. */
        self->lines = AVDC_MALLOC(self->number_of_sets * self->assoc, avdc_cache_line_t);

        /* Flush the cache, this initializes the tag array to a known state */
        avdc_flush_cache(self);

        return 1;
}

void
avdc_print_info(avdark_cache_t *self)
{
        fprintf(stderr, "Cache Info\n");
        fprintf(stderr, "size: %d, assoc: %d, line-size: %d\n",
                self->size, self->assoc, self->block_size);
}

void
avdc_print_internals(avdark_cache_t *self)
{
        int i;

        fprintf(stderr, "Cache Internals\n");
        fprintf(stderr, "size: %d, assoc: %d, line-size: %d\n",
                self->size, self->assoc, self->block_size);

        for (i = 0; i < self->number_of_sets; i++)
                fprintf(stderr, "tag: <0x%.16lx> valid: %d\n",
                        (long unsigned int)self->lines[i].tag,
                        self->lines[i].valid);
}

void
avdc_reset_statistics(avdark_cache_t *self)
{
        self->stat_data_read = 0;
        self->stat_data_read_miss = 0;
        self->stat_data_write = 0;
        self->stat_data_write_miss = 0;
}

avdark_cache_t *
avdc_new(avdc_size_t size, avdc_block_size_t block_size,
         avdc_assoc_t assoc, char *replacement)
{
        avdark_cache_t *self;

        self = AVDC_MALLOC(1, avdark_cache_t);

        memset(self, 0, sizeof(*self));
        self->dbg = 0;
        //self->dbg = 1;
        self->maxCount = 0;
        if ((strcmp(replacement, "LRU") == 0))
       	{
       		self->replacement = LRU;
       	}
       	else if(strcmp(replacement, "RANDOM") == 0)
       	{
       		srand(time(NULL));
       		self->replacement = RANDOM;
       	}
       	else if((strcmp(replacement, "FIFO") == 0))
       	{
       		self->replacement = FIFO;
       	}
        else	
        {
       		AVDC_FREE(self);
        	return NULL;
        }
        
        //avdc_dbg_log(self, "avdc new\n");

        if (!avdc_resize(self, size, block_size, assoc)) {
                AVDC_FREE(self);
                return NULL;
        }

        return self;
}

void
avdc_delete(avdark_cache_t *self)
{
        if (self->lines)
                AVDC_FREE(self->lines);

        AVDC_FREE(self);
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
