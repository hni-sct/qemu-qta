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

#ifndef QTA_H_
#define QTA_H_

#include <inttypes.h>
#include <glib.h>

typedef enum {
    NORMAL,
	START,
	END,
	CALL,
	RETURN
} blocktype;

typedef struct {
	const char* id;
	const char* name;
} routine;

typedef struct {
	const char* id;
	routine* routine;
	GHashTable *out_edges;
	uint64_t pc;
	blocktype blocktype;
} block;

typedef struct {
	block* source;
	block* target;
	const char* source_context;
	const char* target_context;
	uint64_t cycles;
} edge;

typedef struct {
    edge* edge1;     // branch target1 (cond: case1 / uncond)
    edge* edge2;     // branch target2 (cond: case2)
} edge_pair;

void qta_init(const char* filename, const char *logfile);
void qta_query(uint64_t tb_pc_first, uint64_t tb_pc_last);
void qta_exit(void);

#endif /* QTA_H_ */
