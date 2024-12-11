
#include <cstring>
#include <stdint.h>
#include <stdio.h>
#include "app_wifi.h"
#include "core_mqtt_agent.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "transport_interface.h"
#include "network_transport.h"
#include "esp_secure_cert_read.h"
#include "core_mqtt_agent_manager_events.h"
#include "core_mqtt_serializer.h"
#include "core_mqtt_agent_manager.h"

class LoraManager;

typedef struct IncomingPublishCallbackContext
{
    EventGroupHandle_t xMqttEventGroup;
    char pcIncomingPublish[ 4096 ];
} IncomingPublishCallbackContext_t;

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_

class WIFI {
private:	
	static NetworkContext_t xNetworkContext;
	static BaseType_t MQTTinit();
	static SemaphoreHandle_t xMessageIdSemaphore;
	static EventGroupHandle_t xNetworkEventGroup;
    static EventGroupHandle_t xMqttEventGroup;
    static uint32_t ulMessageId;
    static IncomingPublishCallbackContext_t xIncomingPublishCallbackContext;
	static void prvCoreMqttAgentEventHandler( void * pvHandlerArg,esp_event_base_t xEventBase,int32_t lEventId,void * pvEventData );
	static void prvPublishToTopic( MQTTQoS_t xQoS,char * pcTopicName,char * pcPayload,EventGroupHandle_t xMqttEventGroup );
	static void prvSubscribeToTopic(IncomingPublishCallbackContext_t * pxIncomingPublishCallbackContext,MQTTQoS_t xQoS,char * pcTopicFilter,EventGroupHandle_t xMqttEventGroup);
	static EventBits_t prvWaitForEvent( EventGroupHandle_t xMqttEventGroup,EventBits_t uxBitsToWaitFor );
	static void prvPublishCommandCallback( MQTTAgentCommandContext_t * pxCommandContext,MQTTAgentReturnInfo_t * pxReturnInfo );
	static void prvSubscribeCommandCallback( MQTTAgentCommandContext_t * pxCommandContext,MQTTAgentReturnInfo_t * pxReturnInfo );
	static void prvIncomingPublishCallback( void * pvIncomingPublishCallbackContext,MQTTPublishInfo_t * pxPublishInfo );
	uint8_t* extractMessage(const uint8_t* jsonInput);
	LoraManager* lora;
	bool sendHelloMessage;
public:
	WIFI();
	~WIFI();
	void init();
	void MQTTpublish(int temp, int ah,int sh,int ws,int rn, int b1, int b2);
	void MQTTpublishText(uint8_t* number, uint8_t* text, int size);
	void MQTTreceive();
	void MQTTsubscribe();
	void setLoraManager(LoraManager* l);
	void setHelloMessage(bool hello);
};

#endif /* MAIN_WIFI_H_ */
