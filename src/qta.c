/*
 * QTA - Qemu Timing Analyzer
 *
 * Copyright (C) 2021, Peer Adelt <adelt@hni.upb.de>,
 * Heinz Nixdorf Institut/Paderborn University, Paderborn, Germany
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include "qta.h"
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/tree.h>

static GHashTable *routines;
static GHashTable *blocks;
static GQueue *callstack;
static block *blk;
static const char *ctx;
static uint64_t total;
static int active;
static FILE* _log;
static char txt_buf[4096];

static struct tm t_start;
static struct timespec ts_start, ts_end;

static const char *get_prop(xmlXPathObjectPtr xp, int i, const char *p)
{
    return (const char*) xmlGetNoNsProp(xp->nodesetval->nodeTab[i], BAD_CAST p);
}

void qta_init(const char* filename, const char* logfile)
{
    // Get the start timestamp
    time_t t = time(NULL);
    t_start = *localtime(&t);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts_start);

    // Open XML file
    xmlDocPtr doc = xmlParseFile(filename);
    xmlXPathContextPtr xpc = xmlXPathNewContext(doc);
    if (!doc || !xpc) {
        fprintf(stderr, "Error: cannot load QTA time database file '%s'.\n", filename);
        exit(1);
    }
    if(xmlXPathRegisterNs(xpc,  BAD_CAST "qta", BAD_CAST "https://www.hni.uni-paderborn.de/sct") != 0) {
        fprintf(stderr,"Error: unable to register QTA namespace");
        exit(1);
    }
    xmlXPathObjectPtr xp = NULL;

    // Init logfile
    if (logfile) {
        _log = fopen(logfile, "w");
    } else {
        _log = stdout;
    }

    // Init global state variables for QTA
    routines = g_hash_table_new(g_str_hash, g_str_equal);
    blocks = g_hash_table_new(g_str_hash, g_str_equal);
    callstack = g_queue_new();
    blk = NULL;
    ctx = NULL;
    total = 0;
    active = 0;

    // Parse routines
    xp = xmlXPathEvalExpression(BAD_CAST "/qta:TimingAnnotation/Routines/Routine", xpc);
    if (xp && xp->nodesetval) {
        for (int i = 0; i < xp->nodesetval->nodeNr; i++) {
            routine* r = g_new0(routine, 1);
            r->id = get_prop(xp, i, "id");
            r->name = get_prop(xp, i, "name");
            g_hash_table_insert(routines, (gpointer) r->id, r);
        }
    }

    // Parse blocks
    xp = xmlXPathEvalExpression(BAD_CAST "/qta:TimingAnnotation/Blocks/Block", xpc);
    for (int i = 0; i < xp->nodesetval->nodeNr; i++) {
        block* b = g_new0(block, 1);
        b->id = get_prop(xp, i, "id");
        const char *address = get_prop(xp, i, "address");
        if (address) {
            sscanf(address, "0x%" PRIx64, &b->pc);
        }
        const char *routine_id = get_prop(xp, i, "routine");
        if (routine_id) {
            b->routine = g_hash_table_lookup(routines, routine_id);
        }
        const char *blocktype = get_prop(xp, i, "blocktype");
        b->blocktype = NORMAL;
        if (blocktype) {
            if (g_strcmp0(blocktype, "start") == 0) {
                b->blocktype = START;
            } else if (g_strcmp0(blocktype, "end") == 0) {
                b->blocktype = END;
            } else if (g_strcmp0(blocktype, "call") == 0) {
                b->blocktype = CALL;
            } else if (g_strcmp0(blocktype, "return") == 0) {
                b->blocktype = RETURN;
            }
        }
        b->out_edges = g_hash_table_new(g_str_hash, g_str_equal);
        g_hash_table_insert(blocks, (gpointer) b->id, b);
    }

    // Parse edges (also adding out_edges to blocks)
    xp = xmlXPathEvalExpression(BAD_CAST "/qta:TimingAnnotation/Edges/Edge", xpc);
    for (int i = 0; i < xp->nodesetval->nodeNr; i++) {
        edge* e = g_new0(edge, 1);
        e->source_context = get_prop(xp, i, "source_context");
        e->target_context = get_prop(xp, i, "target_context");
        e->source = g_hash_table_lookup(blocks, get_prop(xp, i, "source"));
        e->target = g_hash_table_lookup(blocks, get_prop(xp, i, "target"));
        sscanf(get_prop(xp, i, "cycles"), "%"PRIu64, &e->cycles);

        // Add this edge to the out_hash of its source block
        edge_pair* ep = g_hash_table_lookup(e->source->out_edges, e->source_context);
        if (ep == NULL) {
            // -> If not present: create hash struct and store this edge as edge1
            ep = g_new0(edge_pair, 1);
            ep->edge1 = e;
            ep->edge2 = NULL;
            g_hash_table_insert(e->source->out_edges, (gpointer) e->source_context, ep);
        } else {
            // -> If present: retrieve struct and store this edge as edge2
            if (ep->edge1 == NULL || ep->edge2 != NULL) {
                fprintf(stderr,"Needs clarification: can there be more than 2 outgoing edges per block and context?");
                exit(1);
            }
            ep->edge2 = e;
        }
    }

    // Parse start block and context
    xp = xmlXPathEvalExpression(BAD_CAST "/qta:TimingAnnotation", xpc);
    blk = g_hash_table_lookup(blocks, get_prop(xp, 0, "startBlock"));
    ctx = get_prop(xp, 0, "startContext");

    // Close XML document
    xmlXPathFreeObject(xp);
    xmlXPathFreeContext(xpc);
    xmlFreeDoc(doc);

    // TO DO: Print header
    snprintf(txt_buf, 4096, ""

                            "********************************************************************************\n"
                            "*                                                                              *\n"
                            "*    QTA - Qemu Timing Analyzer                                                *\n"
                            "*                                                                              *\n"
                            "*    Version 0.2                                                               *\n"
                            "*    Copyright (C) 2021, Peer Adelt <adelt@hni.upb.de>, Paderborn University   *\n"
                            "*                                                                              *\n"
                            "********************************************************************************\n"
                            "*                                                                              *\n"
//                            "*          Binary program   : %-39s          *\n"
                            "*          Time database    : %-39s          *\n"
                            "*          Logfile          : %-39s          *\n"
                            "*                                                                              *\n"
                            "*      / -------------------------------------------------------------- \\      *\n"
                            "***** /                       PROGRAM OUTPUT BELOW                       \\ *****\n"
                            "\n",
//                            "CANNOT QUERY FILENAME VIA PLUGIN API",
                            filename,
                            (logfile) ? logfile : "NONE (output is written to console)"
                       );
//            "\n";
    fprintf(_log, "%s", txt_buf);
    if (_log != stdout) {
        printf("%s", txt_buf);
    }
}

void qta_exit(void) {
    // Get the end timestamp
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts_end);
    uint64_t duration_us = (uint64_t) (ts_end.tv_sec - ts_start.tv_sec) * 1000000 + (ts_end.tv_nsec - ts_start
            .tv_nsec) / 1000;
    uint nr_digits_duration = floor(log10(duration_us)) + 1;
    uint nr_digits_total = floor(log10(total)) + 1;

    // Print footer
    snprintf(txt_buf, 4096, "\n\n"
                            "***** \\                       PROGRAM OUTPUT ABOVE                       / *****\n"
                            "*      \\ -------------------------------------------------------------- /      *\n"
                            "*                                                                              *\n"
                            "*          Simulated time    : %-*"PRIu64" cycles%-*s        *\n"
                            "*          Analysis started  : %04d-%02d-%02d %02d:%02d:%02d                             *\n"
                            "*          Analysis duration : %-*"PRIu64" us%-*s        *\n"
                            "*                                                                              *\n"
                            "********************************************************************************\n"
                            "*                                                                              *\n"
                            "*    QTA Timing analysis completed successfully.                               *\n"
                            "*    Qemu will now exit. Goodbye!                                              *\n"
                            "*                                                                              *\n"
                            "********************************************************************************\n",
                            nr_digits_total, total, 33 - nr_digits_total,"",
                            t_start.tm_year + 1900, t_start.tm_mon + 1, t_start.tm_mday,
                            t_start.tm_hour, t_start.tm_min, t_start.tm_sec,
                            nr_digits_duration, duration_us, 37 - nr_digits_duration, "");
    fprintf(_log, "%s", txt_buf);
    if (_log != stdout) {
        printf("%s", txt_buf);
    }

    // Close logfile
    fflush(_log);
    fflush(stdout);
    if (_log != stdout) {
        fclose(_log);
    }
}

typedef struct {
    routine *r_caller;
    routine *r_callee;
    const char *ctx_caller;
    const char *ctx_callee;
    uint64_t ret_addr;
} callstack_entry;

static void qta_takeedge(edge* edge)
{
    blk = edge->target;
    ctx = edge->target_context;
    total += edge->cycles;

    if (edge->target->blocktype == RETURN) {
        // UPDATE CALL STACK:
        callstack_entry *entry = (callstack_entry *) g_queue_pop_head(callstack);
        // PLAUSIBILITY CHECK:
        if (g_strcmp0(edge->source_context, entry->ctx_callee)) {
            fprintf(stderr, "\t[ERROR]: CALLEE STACK CONTEXT MISMATCH!\n");
            exit(1);
        }
        if (g_strcmp0(edge->target_context, entry->ctx_caller)) {
            fprintf(stderr, "\t[ERROR]: CALLER STACK CONTEXT MISMATCH!\n");
            exit(1);
        }
        if (edge->target->pc != entry->ret_addr) {
            fprintf(stderr, "\t[ERROR]: CALL STACK ADDR MISMATCH!\n");
            exit(1);
        }

        fprintf(_log, "\tRETURN %s @ 0x%08"PRIx64"",
               entry->r_caller ? entry->r_caller->name : "?",
               entry->ret_addr);
    } else if (edge->source->blocktype == CALL) {
        // UPDATE CALL STACK:
        callstack_entry *entry = g_new0(callstack_entry, 1);
        entry->r_caller = edge->source->routine;
        entry->r_callee = edge->target->routine;
        entry->ctx_caller = edge->source_context;
        entry->ctx_callee = edge->target_context;
        entry->ret_addr = edge->source->pc;
        g_queue_push_head(callstack, (gpointer) entry);
        fprintf(_log, "\tCALL %s @ 0x%08"PRIx64"",
               entry->r_callee ? entry->r_callee->name : "?",
               entry->ret_addr);
//    } else if (edge->target->blocktype == END) {
//        fprintf(_log, "\tEND %s", (edge->source->routine &&
//        edge->source->routine->name) ? edge->source->routine->name : "?");
//    } else if (edge->source->blocktype == START) {
//	    fprintf(_log, "\tSTART %s", (edge->source->routine &&
//	    edge->source->routine->name) ? edge->source->routine->name : "?");
    } else {
        fprintf(_log, "\tEDGE");
    }

    fprintf(_log, " (%s -> %s)", edge->source->id, edge->target->id);
    if (edge->cycles) {
        fprintf(_log, "\t(Time: %3"PRIu64" Cycles)", edge->cycles);
    }
    fprintf(_log, "\n");
}

/* Get the current unconditional jump target, if applicable.
   Returns NULL for conditional jumps of if end of timing graph. */
static edge *get_edge_unconditional()
{

    edge_pair *ep = g_hash_table_lookup(blk->out_edges, ctx);
    if (ep) {
        if (ep->edge1 && !ep->edge2) {
            return ep->edge1;
        } else if (!ep->edge1 && ep->edge2) {
            return ep->edge2;
        }
    }
    return NULL;
}

static edge *get_edge_for_pc(uint64_t pc)
{
    edge_pair *ep = g_hash_table_lookup(blk->out_edges, ctx);
    if (ep) {
        if (ep->edge1 && ep->edge1->target->pc && ep->edge1->target->pc == pc) {
            return ep->edge1;
        } else if (ep->edge2 && ep->edge2->target->pc && ep->edge2->target->pc == pc) {
            return ep->edge2;
        }
    }
    return NULL;
}

static edge* qta_try_next_edge(uint64_t from, uint64_t to)
{
    // Get the (up to 2) outgoing edges for this context and block:
    edge *e = get_edge_unconditional();
    if (e) {
        if (e->target->blocktype == NORMAL &&
            (e->target->pc > to || e->target->pc < from)) {
            return NULL;
        } else {
            return e;
        }
    }
    return NULL;
}

static bool qta_is_return_valid(edge* e) {
    callstack_entry *entry = (callstack_entry *) g_queue_peek_head(callstack);
    if (e != NULL &&
        entry != NULL &&
        g_strcmp0(e->source_context, entry->ctx_callee) == 0 &&
        g_strcmp0(e->target_context, entry->ctx_caller) == 0 &&
        e->target->pc == entry->ret_addr) {
        return TRUE;
    }
    return FALSE;
}

void qta_query(uint64_t tb_pc_first, uint64_t tb_pc_last) {

    if (active == 0 && blk->pc && blk->pc == tb_pc_first)
    {
        fprintf(_log, "[QTA]\tSimulation started\n");
//        fprintf(_log, "\tCurrent Block: %s (Type: %d)\n", blk->id, blk->blocktype);
        active = 1;
    }

    if (active == 1) {

        fprintf(_log, "\n[QTA]\tExecute TB\t[ 0x%08"PRIx64" : 0x%08"PRIx64" ]\n", tb_pc_first, tb_pc_last);

        // Get to the first aiT Block that is inside current TB
        edge *e = get_edge_for_pc(tb_pc_first);
        while (e == NULL) {
            // Stategy: As long as we cannot reach TB start, try to RETURN further

            // Phase 1: Take the only reachable END node.
            //          (Note: no block can branch to 2 END blocks)
            edge_pair *ep = g_hash_table_lookup(blk->out_edges, ctx);
            if (ep->edge1 && ep->edge1->target->blocktype == END) {
                qta_takeedge(ep->edge1);
            } else if (ep->edge2 && ep->edge2->target->blocktype == END) {
                qta_takeedge(ep->edge2);
            }

            // // Phase 2: Choose the correct RETURN node from here.
            // //          (Correct by means of the call stack top...)
            ep = g_hash_table_lookup(blk->out_edges, ctx);
            if (qta_is_return_valid(ep->edge1)) {
                qta_takeedge(ep->edge1);
            } else if (qta_is_return_valid(ep->edge2)) {
                qta_takeedge(ep->edge2);
            } else {
                fprintf(stderr, "ERROR: Could not find matching RETURN block!\n");
                exit(1);
            }

            // Now check again if we can now reach the TB start from current RETURN block...
            e = get_edge_for_pc(tb_pc_first);
        }

        // Take edge to the beginning of the TBs tb_pc_first range
        qta_takeedge(e);
        // Proceed all included aiT Blocks
        edge *next = qta_try_next_edge(tb_pc_first, tb_pc_last);
        while (next) {
            qta_takeedge(next);
            next = qta_try_next_edge(tb_pc_first, tb_pc_last);
        }

        edge_pair *ep = g_hash_table_lookup(blk->out_edges, ctx);
        if (!ep) {
            fprintf(_log, "\n[QTA]\tSimulation finished\t\t(Total cycles: %"PRIu64")", total);
            exit(0);
        }
    }
}
