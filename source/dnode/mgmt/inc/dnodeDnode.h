/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TD_DNODE_DNODE_H_
#define _TD_DNODE_DNODE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "dnodeInt.h"

int32_t dnodeInitDnode();
void    dnodeCleanupDnode();
void    dnodeProcessStatusRsp(SRpcMsg *pMsg);
void    dnodeProcessStartupReq(SRpcMsg *pMsg);
void    dnodeProcessConfigDnodeReq(SRpcMsg *pMsg);

int32_t dnodeInitConfig();
void    dnodeCleanupConfig();

void    dnodeUpdateCfg(SDnodeCfg *data);
void    dnodeUpdateDnodeEps(SDnodeEps *data);
void    dnodeUpdateMnodeEps(SRpcEpSet *pEpSet);
int32_t dnodeGetDnodeId();
int64_t dnodeGetClusterId();
void    dnodeGetEp(int32_t dnodeId, char *epstr, char *fqdn, uint16_t *port);

void dnodeGetEpSetForPeer(SRpcEpSet *epSet);
void dnodeSendRedirectMsg(SRpcMsg *rpcMsg, bool forShell);


#ifdef __cplusplus
}
#endif

#endif /*_TD_DNODE_DNODE_H_*/