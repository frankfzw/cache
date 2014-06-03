/**
 * Glue code for running the AvDark cache simulator in Pin.
 */


#include "pin.H"

#include <iostream>
#include <fstream>
#include <string.h>

#include <stdlib.h>

#include <sys/time.h>

extern "C" {
#include "cache.h"
}

KNOB<string> knob_output(KNOB_MODE_WRITEONCE,    "pintool",
                         "o", "cache.out", "specify log file name");

KNOB<UINT32> knob_l1_size(KNOB_MODE_WRITEONCE, "pintool",
		       "s1", "8388608", "L1 Cache size (bytes)");
KNOB<UINT32> knob_l2_size(KNOB_MODE_WRITEONCE, "pintool",
		       "s2", "8388608", "L2 Cache size (bytes)");
KNOB<UINT32> knob_associativity(KNOB_MODE_WRITEONCE, "pintool",
				"a", "1", "Cache associativity");
KNOB<UINT32> knob_line_size(KNOB_MODE_WRITEONCE, "pintool",
			    "l", "64", "Cache line size");
KNOB<string> knob_replacement(KNOB_MODE_WRITEONCE, "pintool",
			    "replace", "LRU", "Cache replacement policy");

static avdark_cache_t *avdc1 = NULL;
static avdark_cache_t *avdc2 = NULL;

static int total_read = 0;
static int total_read_miss = 0;
static int total_write = 0;
static int total_write_miss = 0;

/**
 * Memory access callback. Will be called for every memory access
 * executed by the the target application.
 */
static VOID
simulate_access(VOID *addr, UINT32 access_type)
{

        /* NOTE: We deliberately ignore the fact that addr may
         * straddle multiple cache lines */
        //avdc_access(avdc, (avdc_pa_t)addr, (avdc_access_type_t)access_type);
        
        //modified 2-level-cache
        avdc_pa_t pa = (avdc_pa_t)addr;
        avdc_access_type_t type = (avdc_access_type_t)access_type;
        
        int hit = two_level_access(avdc1, avdc2, pa, type);
        
        
        switch (type)
        {
	case AVDC_READ:
		total_read ++;
		total_read_miss += 1 - hit;
		break;
	case AVDC_WRITE:
		total_write ++;
		total_write_miss += 1 - hit;
		break;
        }
        
        

}

/**
 * PIN instrumentation callback, called for every new instruction that
 * PIN discovers in the application. This function is used to
 * instrument code blocks by inserting calls to instrumentation
 * functions.
 */
static VOID
instruction(INS ins, VOID *not_used)
{
        UINT32 no_ops = INS_MemoryOperandCount(ins);

        for (UINT32 op = 0; op < no_ops; op++) {
                //const UINT32 size = INS_MemoryOperandSize(ins, op);
                //const bool is_rd = INS_MemoryOperandIsRead(ins, op);
                const bool is_wr = INS_MemoryOperandIsWritten(ins, op);
                const UINT32 atype = is_wr ? AVDC_WRITE : AVDC_READ;


                INS_InsertCall(ins, IPOINT_BEFORE,
                               (AFUNPTR)simulate_access,
                               IARG_MEMORYOP_EA, op,
                               IARG_UINT32, atype,
                               IARG_END); 
        }
}

/**
 * PIN fini callback. Called after the target application has
 * terminated. Used to print statistics and do cleanup.
 */
static VOID
fini(INT32 code, VOID *v)
{
        std::ofstream out(knob_output.Value().c_str());
        int accesses1 = avdc1->stat_data_read + avdc1->stat_data_write;
        int misses1 = avdc1->stat_data_read_miss + avdc1->stat_data_write_miss;

        out << "L1 Cache statistics:" << endl;
        out << "  Writes: " << avdc1->stat_data_write << endl;
        out << "  Write Misses: " << avdc1->stat_data_write_miss << endl;
        out << "  Reads: " << avdc1->stat_data_read << endl;
        out << "  Read Misses: " << avdc1->stat_data_read_miss << endl;
        out << "  Misses: " << misses1 << endl;
        out << "  Accesses: " << accesses1 << endl;
        out << "  Miss Ratio: " << ((100.0 * misses1) / accesses1) << "%" << endl;
        
        int accesses2 = avdc2->stat_data_read + avdc2->stat_data_write;
        int misses2 = avdc2->stat_data_read_miss + avdc2->stat_data_write_miss;

        out << "L2 Cache statistics:" << endl;
        out << "  Writes: " << avdc2->stat_data_write << endl;
        out << "  Write Misses: " << avdc2->stat_data_write_miss << endl;
        out << "  Reads: " << avdc2->stat_data_read << endl;
        out << "  Read Misses: " << avdc2->stat_data_read_miss << endl;
        out << "  Misses: " << misses2 << endl;
        out << "  Accesses: " << accesses2 << endl;
        out << "  Miss Ratio: " << ((100.0 * misses2) / accesses2) << "%" << endl;
        
        int total_access = total_read + total_write;
        int total_miss = total_read_miss + total_write_miss;
        out << "2 Level Cache statistics:" << endl;
        out << "  Writes: " <<total_write << endl;
        out << "  Write Misses: " << total_write_miss << endl;
        out << "  Reads: " << total_read << endl;
        out << "  Read Misses: " << total_read_miss << endl;
        out << "  Misses: " << total_miss << endl;
        out << "  Accesses: " << total_access << endl;
        out << "  Miss Ratio: " << ((100.0 * total_miss) / total_access) << "%" << endl;

        avdc_delete(avdc1);
        avdc_delete(avdc2);
}

static int
usage()
{
        cerr <<
                "This is the Advanced Computer Architecture online analysis tool\n"
                "\n";

        cerr << KNOB_BASE::StringKnobSummary();

        cerr << endl;

        return -1;
}

int main(int argc, char *argv[])
{
        if (PIN_Init(argc, argv))
                return usage();

        avdc_size_t l1_size = knob_l1_size.Value();
        avdc_size_t l2_size = knob_l2_size.Value();
        avdc_block_size_t block_size = knob_line_size.Value();
        avdc_assoc_t assoc = knob_associativity.Value();
        
        //add replacement
        int len = strlen(knob_replacement.Value().c_str());
        char *replacement = (char *) malloc(len * sizeof(char));
        strcpy(replacement, knob_replacement.Value().c_str());

        avdc1 = avdc_new(l1_size, block_size, assoc, replacement);
        avdc2 = avdc_new(l2_size, block_size, assoc, replacement);
        
        if (!avdc1 || !avdc2) {
                cerr << "Failed to initialize the AvDark cache simulator." << endl;
                return -1;
        }

        INS_AddInstrumentFunction(instruction, 0);
        PIN_AddFiniFunction(fini, 0);

        PIN_StartProgram();
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
