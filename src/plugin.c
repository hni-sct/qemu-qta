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

#include <stdio.h>
#include <glib.h>
#include "qta.h"
#include <qemu-plugin.h>

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

typedef struct {
    uint64_t pc_first;
    uint64_t pc_last;
} tb_info;

static void plugin_exit(qemu_plugin_id_t id, void *p)
{
    qta_exit();
}

// Search time annotation for executed TB and increase CycleCounter
static void vcpu_tb_exec(unsigned int cpu_index, void *udata)
{
	tb_info *info = (tb_info *) udata;
	qta_query(info->pc_first, info->pc_last);
}

// Get PC and byte length of instructions of each TB
static void vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
	tb_info *info = g_new0(tb_info, 1);
	info->pc_first = qemu_plugin_tb_vaddr(tb);
	size_t n_insns = qemu_plugin_tb_n_insns(tb);
	info->pc_last = qemu_plugin_insn_vaddr(qemu_plugin_tb_get_insn(tb, n_insns-1));
    qemu_plugin_register_vcpu_tb_exec_cb(tb, vcpu_tb_exec,QEMU_PLUGIN_CB_NO_REGS, (void *) info);
}

QEMU_PLUGIN_EXPORT int qemu_plugin_install(qemu_plugin_id_t id,
                                           const qemu_info_t *info,
                                           int argc, char **argv)
{
    const char* qtdb = NULL;
    const char* logfile = NULL;
    switch (argc) {
        case 0:
            fprintf(stderr, "ERROR: No qtdb file specified!\n");
            exit(1);
        case 2:
            logfile = argv[1];
        case 1:
            qtdb = argv[0];
            break;
    }
    qta_init(qtdb, logfile);
    qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);
    return 0;
}
