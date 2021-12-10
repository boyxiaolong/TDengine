#include "tglobal.h"
#include "clientInt.h"
#include "tdef.h"
#include "tep.h"
#include "tmsgtype.h"
#include "tref.h"
#include "tscLog.h"

static int initEpSetFromCfg(const char *firstEp, const char *secondEp, SRpcCorEpSet *pEpSet);

static bool stringLengthCheck(const char* str, size_t maxsize) {
  if (str == NULL) {
    return false;
  }

  size_t len = strlen(str);
  if (len <= 0 || len > maxsize) {
    return false;
  }

  return true;
}

static bool validateUserName(const char* user) {
  return stringLengthCheck(user, TSDB_USER_LEN - 1);
}

static bool validatePassword(const char* passwd) {
  return stringLengthCheck(passwd, TSDB_KEY_LEN - 1);
}

static bool validateDbName(const char* db) {
  return stringLengthCheck(db, TSDB_DB_NAME_LEN - 1);
}

static STscObj* taosConnectImpl(const char *ip, const char *user, const char *auth, const char *db, uint16_t port, __taos_async_fn_t fp, void *param);

TAOS *taos_connect_internal(const char *ip, const char *user, const char *pass, const char *auth, const char *db, uint16_t port) {
  if (!validateUserName(user)) {
    terrno = TSDB_CODE_TSC_INVALID_USER_LENGTH;
    return NULL;
  }

  char tmp[TSDB_DB_NAME_LEN] = {0};
  if (db != NULL) {
    if(!validateDbName(db)) {
      terrno = TSDB_CODE_TSC_INVALID_DB_LENGTH;
      return NULL;
    }

    tstrncpy(tmp, db, sizeof(tmp));
    strdequote(tmp);
  }

  char secretEncrypt[32] = {0};
  if (auth == NULL) {
    if (!validatePassword(pass)) {
      terrno = TSDB_CODE_TSC_INVALID_PASS_LENGTH;
      return NULL;
    }

    taosEncryptPass((uint8_t *)pass, strlen(pass), secretEncrypt);
  } else {
    tstrncpy(secretEncrypt, auth, tListLen(secretEncrypt));
  }

  SRpcCorEpSet epSet;
  if (ip) {
    if (initEpSetFromCfg(ip, NULL, &epSet) < 0) {
      return NULL;
    }

    if (port) {
      epSet.epSet.port[0] = port;
    }
  } else {
    if (initEpSetFromCfg(tsFirst, tsSecond, &epSet) < 0) {
      return NULL;
    }
  }

  return taosConnectImpl(ip, user, auth, db, port, NULL, NULL);
}

int initEpSetFromCfg(const char *firstEp, const char *secondEp, SRpcCorEpSet *pEpSet) {
  pEpSet->version = 0;

  // init mgmt ip set
  SEpSet *mgmtEpSet   = &(pEpSet->epSet);
  mgmtEpSet->numOfEps = 0;
  mgmtEpSet->inUse    = 0;

  if (firstEp && firstEp[0] != 0) {
    if (strlen(firstEp) >= TSDB_EP_LEN) {
      terrno = TSDB_CODE_TSC_INVALID_FQDN;
      return -1;
    }

    taosGetFqdnPortFromEp(firstEp, mgmtEpSet->fqdn[0], &(mgmtEpSet->port[0]));
    mgmtEpSet->numOfEps++;
  }

  if (secondEp && secondEp[0] != 0) {
    if (strlen(secondEp) >= TSDB_EP_LEN) {
      terrno = TSDB_CODE_TSC_INVALID_FQDN;
      return -1;
    }

    taosGetFqdnPortFromEp(secondEp, mgmtEpSet->fqdn[mgmtEpSet->numOfEps], &(mgmtEpSet->port[mgmtEpSet->numOfEps]));
    mgmtEpSet->numOfEps++;
  }

  if (mgmtEpSet->numOfEps == 0) {
    terrno = TSDB_CODE_TSC_INVALID_FQDN;
    return -1;
  }

  return 0;
}

STscObj* taosConnectImpl(const char *ip, const char *user, const char *auth, const char *db, uint16_t port, __taos_async_fn_t fp, void *param) {
  if (taos_init() != TSDB_CODE_SUCCESS) {
    return NULL;
  }

  STscObj *pObj = createTscObj(user, auth, ip, port);
  if (NULL == pObj) {
    terrno = TSDB_CODE_TSC_OUT_OF_MEMORY;
    return pObj;
  }

  //  void *pRpcObj = NULL;
  //
  //  char rpcKey[512] = {0};
  //  snprintf(rpcKey, sizeof(rpcKey), "%s:%s:%s:%d", user, auth, ip, port);
  //  if (tscAcquireRpc(rpcKey, user, auth, &pRpcObj) != 0) {
  //    terrno = TSDB_CODE_RPC_NETWORK_UNAVAIL;
  //    return NULL;
  //  }

  SRequestObj *pRequest = createRequest(pObj, fp, param, TSDB_SQL_CONNECT);
  if (pRequest == NULL) {
    destroyTscObj(pObj);
    terrno = TSDB_CODE_TSC_OUT_OF_MEMORY;
  }

  //    tscBuildAndSendRequest(pRequest, NULL);
  //    tsem_wait(&pRequest->body.rspSem);
  if (pRequest->code != TSDB_CODE_SUCCESS) {
    const char *errorMsg = (pRequest->code == TSDB_CODE_RPC_FQDN_ERROR)
                               ? taos_errstr(pRequest)
                               : tstrerror(terrno);

    printf("connect failed, reason: %s\n\n", errorMsg);

    taos_free_result(pRequest);
    taos_close(pObj);
    return NULL;
  }

//  tscDebug("0x%"PRIx64" connection is opening, rpcObj: %p, dnodeConn:%p", pObj, pObj->pRpcObj,
//      pObj->pRpcObj->pDnodeConn);
  destroyRequest(pRequest);
  return pObj;
}