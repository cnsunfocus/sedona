#include <stdio.h>
#include "MQTTClient.h"
#include "communityMQTT_common.h"

#include <stdio.h>
#include <signal.h>
#include <memory.h>

#include <sys/time.h>

#include "sedona.h"

// native method slots
Cell communityMQTT_Message_doPublish(SedonaVM* vm, Cell* params)
{
  SessionHandle * pSession = (SessionHandle *)params[0].aval;
  uint8_t * topic = params[1].aval;
  uint8_t * payload = params[2].aval;
  int32_t payload_len = params[3].ival;
  int32_t qos = params[4].ival;
  
  Payload * pPayload = malloc(sizeof(Payload));
  pPayload->type = PublishTask;

  PublishData * pData = malloc(sizeof(PublishData));
  pData->topic = malloc(strlen(topic)+1);
  strcpy(pData->topic, topic);
  pData->payload = malloc(payload_len);
  memcpy(pData->payload, payload, payload_len);
  pData->payload_len = payload_len;
  pData->qos = qos;
  pPayload->pPublishData = pData;

  pushPayload(pSession, pPayload);
  return nullCell;
}

Cell communityMQTT_Message_doSubscribe(SedonaVM* vm, Cell* params)
{
  SessionHandle * pSession = (SessionHandle *)params[0].aval;
  uint8_t * topic = params[1].aval;
  int32_t qos = params[2].ival;
  
  Payload * pPayload = malloc(sizeof(Payload));
  pPayload->type = SubscribeTask;
  SubscribeData * pData = malloc(sizeof(SubscribeData));
  pData->topic = malloc(strlen(topic)+1);
  strcpy(pData->topic, topic);
  pData->qos = qos;
  pPayload->pSubscribeData = pData;

  pushPayload(pSession, pPayload);
  return nullCell;
}

Cell communityMQTT_Message_fetchData(SedonaVM* vm, Cell* params)
{
  SessionHandle * pSession = (SessionHandle *)params[0].aval;
  uint8_t * topic = params[1].aval;
  uint8_t * buf = params[2].aval;
  int32_t length = params[3].ival;

  SubscribeResponse * pResponse = NULL;
  HASH_FIND_STR(pSession->pResponse, topic, pResponse);
  if (!pResponse)
    return falseCell;

  bool changed = false;
  int32_t strLen = strlen(buf);
  if (pResponse->payload_len < strLen || (pResponse->payload_len > strLen && pResponse->payload_len < length)) 
    changed = true;
  else if (pResponse->payload_len == strLen)
    changed = memcmp(pResponse->payload, buf, strLen) != 0;
  else // pResponse->payload_len >= length
    changed = memcmp(pResponse->payload, buf, length-1) != 0;

  //only return when data changed
  if (!changed)
    return falseCell;

  int32_t minLen = pResponse->payload_len > length-1 ? length-1 : pResponse->payload_len;
  if (minLen < 0)
    return falseCell;

  strncpy(buf, pResponse->payload, minLen);
  buf[minLen] = 0;
  return trueCell;
}