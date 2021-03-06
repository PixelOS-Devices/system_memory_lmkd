/*
 *  Copyright 2018 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/cdefs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

#include <cutils/sockets.h>
#include <liblmkd_utils.h>
#include <processgroup/processgroup.h>

int lmkd_connect() {
    return socket_local_client("lmkd",
                               ANDROID_SOCKET_NAMESPACE_RESERVED,
                               SOCK_SEQPACKET | SOCK_CLOEXEC);
}

int lmkd_register_proc(int sock, struct lmk_procprio *params) {
    LMKD_CTRL_PACKET packet;
    size_t size;
    int ret;

    size = lmkd_pack_set_procprio(packet, params);
    ret = TEMP_FAILURE_RETRY(write(sock, packet, size));

    return (ret < 0) ? -1 : 0;
}

int lmkd_unregister_proc(int sock, struct lmk_procremove *params) {
    LMKD_CTRL_PACKET packet;
    size_t size;
    int ret;

    size = lmkd_pack_set_procremove(packet, params);
    ret = TEMP_FAILURE_RETRY(write(sock, packet, size));

    return (ret < 0) ? -1 : 0;
}

enum update_props_result lmkd_update_props(int sock) {
    LMKD_CTRL_PACKET packet;
    int size;

    size = lmkd_pack_set_update_props(packet);
    if (TEMP_FAILURE_RETRY(write(sock, packet, size)) < 0) {
        return UPDATE_PROPS_SEND_ERR;
    }

    size = TEMP_FAILURE_RETRY(read(sock, packet, CTRL_PACKET_MAX_SIZE));
    if (size < 0) {
        return UPDATE_PROPS_RECV_ERR;
    }

    if (size != 2 * sizeof(int) || lmkd_pack_get_cmd(packet) != LMK_UPDATE_PROPS) {
        return UPDATE_PROPS_FORMAT_ERR;
    }

    struct lmk_update_props_reply params;
    lmkd_pack_get_update_props_repl(packet, &params);

    return params.result == 0 ? UPDATE_PROPS_SUCCESS : UPDATE_PROPS_FAIL;
}

int create_memcg(uid_t uid, pid_t pid) {
    return createProcessGroup(uid, pid, true) == 0 ? 0 : -1;
}

