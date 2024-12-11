
#include "subscription_manager.h"
#include "wifi_provisioning/manager.h"
#include "LoraManager.h"
#include <cstring>


#define CORE_MQTT_AGENT_CONNECTED_BIT              ( 1 << 0 )
#define CORE_MQTT_AGENT_OTA_NOT_IN_PROGRESS_BIT    ( 1 << 1 )

#define MQTT_INCOMING_PUBLISH_RECEIVED_BIT         ( 1 << 0 )
#define MQTT_PUBLISH_COMMAND_COMPLETED_BIT         ( 1 << 1 )
#define MQTT_SUBSCRIBE_COMMAND_COMPLETED_BIT       ( 1 << 2 )
#define MQTT_UNSUBSCRIBE_COMMAND_COMPLETED_BIT     ( 1 << 3 )

extern "C" {
    extern const char root_cert_auth_start[] asm("_binary_root_cert_auth_crt_start");
    extern const char root_cert_auth_end[] asm("_binary_root_cert_auth_crt_end");
    extern MQTTAgentContext_t xGlobalMqttAgentContext;
}
NetworkContext_t WIFI::xNetworkContext;
	
struct MQTTAgentCommandContext
{
    MQTTStatus_t xReturnStatus;
    EventGroupHandle_t xMqttEventGroup;
    IncomingPublishCallbackContext_t * pxIncomingPublishCallbackContext;
    void * pArgs;
};

EventGroupHandle_t WIFI::xNetworkEventGroup;
SemaphoreHandle_t WIFI::xMessageIdSemaphore;
EventGroupHandle_t WIFI::xMqttEventGroup;
IncomingPublishCallbackContext_t WIFI::xIncomingPublishCallbackContext;
uint32_t WIFI::ulMessageId = 0;


WIFI::WIFI() 
{
	sendHelloMessage = false;
}

WIFI::~WIFI() 
{

}

void WIFI::init()
{
	ESP_LOGI("WIFI","Inicializando wifi");
	
	esp_err_t xEspErrRet;
	
	//xEspErrRet = wifi_prov_mgr_reset_provisioning();
		
	xEspErrRet = nvs_flash_init();

    if( ( xEspErrRet == ESP_ERR_NVS_NO_FREE_PAGES ) ||
        ( xEspErrRet == ESP_ERR_NVS_NEW_VERSION_FOUND ) )
    {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK( nvs_flash_erase() );

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK( nvs_flash_init() );
    }
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    
	MQTTinit();
	
	
    
    app_wifi_init();
    app_wifi_start( POP_TYPE_MAC );
}

BaseType_t WIFI::MQTTinit()
{
	/* This is returned by this function. */
    BaseType_t xRet = pdPASS;

    /* This is used to store the error return of ESP-IDF functions. */
    esp_err_t xEspErrRet;

    /* Verify that the MQTT endpoint and thing name have been configured by the
     * user. */
    if( strlen( CONFIG_GRI_MQTT_ENDPOINT ) == 0 )
    {
        ESP_LOGE( "WIFI", "Empty endpoint for MQTT broker. Set endpoint by "
                       "running idf.py menuconfig, then Golden Reference Integration -> "
                       "Endpoint for MQTT Broker to use." );
        xRet = pdFAIL;
    }

    if( strlen( CONFIG_GRI_THING_NAME ) == 0 )
    {
        ESP_LOGE( "WIFI", "Empty thingname for MQTT broker. Set thing name by "
                       "running idf.py menuconfig, then Golden Reference Integration -> "
                       "Thing name." );
        xRet = pdFAIL;
    }

    /* Initialize network context. */

    xNetworkContext.pcHostname = CONFIG_GRI_MQTT_ENDPOINT;
    xNetworkContext.xPort = CONFIG_GRI_MQTT_PORT;

    /* Get the device certificate from esp_secure_crt_mgr and put into network
     * context. */
	xEspErrRet = esp_secure_cert_get_device_cert(const_cast<char **>(&xNetworkContext.pcClientCert), 
	                                             &xNetworkContext.pcClientCertSize);

    if( xEspErrRet == ESP_OK )
    {
        #if CONFIG_GRI_OUTPUT_CERTS_KEYS
            ESP_LOGI( "WIFI", "\nDevice Cert: \nLength: %" PRIu32 "\n%s",
                      xNetworkContext.pcClientCertSize,
                      xNetworkContext.pcClientCert );
        #endif /* CONFIG_GRI_OUTPUT_CERTS_KEYS */
    }
    else
    {
        ESP_LOGE( "WIFI", "Error in getting device certificate. Error: %s",
                  esp_err_to_name( xEspErrRet ) );

        xRet = pdFAIL;
    }

    /* Putting the Root CA certificate into the network context. */
    xNetworkContext.pcServerRootCA = root_cert_auth_start;
    xNetworkContext.pcServerRootCASize = root_cert_auth_end - root_cert_auth_start;

    if( xEspErrRet == ESP_OK )
    {
        #if CONFIG_GRI_OUTPUT_CERTS_KEYS
//            ESP_LOGI( "WIFI", "\nCA Cert: \nLength: %" PRIu32 "\n%s",
//                      xNetworkContext.pcServerRootCASize,
//                      xNetworkContext.pcServerRootCA );
        #endif /* CONFIG_GRI_OUTPUT_CERTS_KEYS */
    }
    else
    {
        ESP_LOGE( "WIFI", "Error in getting CA certificate. Error: %s",
                  esp_err_to_name( xEspErrRet ) );

        xRet = pdFAIL;
    }

    #if CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL

        /* If the digital signature peripheral is being used, get the digital
         * signature peripheral context from esp_secure_crt_mgr and put into
         * network context. */

        xNetworkContext.ds_data = esp_secure_cert_get_ds_ctx();

        if( xNetworkContext.ds_data == NULL )
        {
            ESP_LOGE( TAG, "Error in getting digital signature peripheral data." );
            xRet = pdFAIL;
        }
    #else /* if CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL */
    	char *tempClientKey = nullptr;
        xEspErrRet = esp_secure_cert_get_priv_key( &tempClientKey,
                                                   &xNetworkContext.pcClientKeySize );
                                                   
        xNetworkContext.pcClientKey = tempClientKey;

        if( xEspErrRet == ESP_OK )
        {
            #if CONFIG_GRI_OUTPUT_CERTS_KEYS
                ESP_LOGI( "WIFI", "\nPrivate Key: \nLength: %" PRIu32 "\n%s",
                          xNetworkContext.pcClientKeySize,
                          xNetworkContext.pcClientKey );
            #endif /* CONFIG_GRI_OUTPUT_CERTS_KEYS */
        }
        else
        {
            ESP_LOGE( "WIFI", "Error in getting private key. Error: %s",
                      esp_err_to_name( xEspErrRet ) );

            xRet = pdFAIL;
        }
    #endif /* CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL */

    xNetworkContext.pxTls = NULL;
    xNetworkContext.xTlsContextSemaphore = xSemaphoreCreateMutex();

    if( xNetworkContext.xTlsContextSemaphore == NULL )
    {
        ESP_LOGE( "WIFI", "Not enough memory to create TLS semaphore for global "
                       "network context." );

        xRet = pdFAIL;
    }
    
//    BaseType_t xResult;
//    xResult = xCoreMqttAgentManagerStart( &xNetworkContext );
//
//    if( xResult != pdPASS )
//    {
//        ESP_LOGE( "WIFI", "Failed to initialize and start coreMQTT-Agent network "
//                       "manager." );
//
//        configASSERT( xResult == pdPASS );
//	}
	BaseType_t xResult;
	xResult = xCoreMqttAgentManagerStart( &xNetworkContext );

    if( xResult != pdPASS )
    {
        ESP_LOGE( "WIFI", "Failed to initialize and start coreMQTT-Agent network "
                       "manager." );

        configASSERT( xResult == pdPASS );
    }
    
    xMessageIdSemaphore = xSemaphoreCreateMutex();
    xNetworkEventGroup = xEventGroupCreate();
    xCoreMqttAgentManagerRegisterHandler( prvCoreMqttAgentEventHandler );

    /* Initialize the coreMQTT-Agent event group. */
    xEventGroupSetBits( xNetworkEventGroup,
                        CORE_MQTT_AGENT_OTA_NOT_IN_PROGRESS_BIT );

    xMqttEventGroup = xEventGroupCreate();
    xIncomingPublishCallbackContext.xMqttEventGroup = xMqttEventGroup;

    return xRet;
}

void WIFI::MQTTpublishText(uint8_t* number, uint8_t* text, int size) {
	char pcTopicName[] = "SOS/OUT";
	char pcPayload[4096];
	char helloMessage[2048];
	
	if (sendHelloMessage) {
		const char *location = "Pico do Paraná";
		const char *latitude = "-25.252657";
		const char *longitude = "-48.812578";
		const char *format = "Hello, I am SafeClimb. If you are receiving this message, it's because someone needs your help. Here are some standard details: \n\nLocation: %s, \nCoordinates: %s %s,\n\nBelow is the message from the person requiring your attention:\n\n%s";
		
		if (text[size - 1] != '\0') {
		    text[size - 1] = '\0'; 
		}
		
		int requiredSize = snprintf(helloMessage, sizeof(helloMessage), format, location, latitude, longitude, (char*)text);
		if (requiredSize >= sizeof(helloMessage)) {
		    ESP_LOGE("WIFI", "Hello message too large. Truncated.");
		}
		
		printf("Hello Message: %s\n", helloMessage);

		sendHelloMessage = false;
		
		snprintf(pcPayload, sizeof(pcPayload),
	         "{\"message\": \"%s\", \"toNumber\": \"%s\"}", helloMessage, (char*)number);
	}
	else
	{
		snprintf(pcPayload, sizeof(pcPayload),
	         "{\"message\": \"%s\", \"toNumber\": \"%s\"}", (char*)text, (char*)number);
	}
	
	prvPublishToTopic((MQTTQoS_t)1, pcTopicName, pcPayload, xMqttEventGroup);
	
	ESP_LOGI("WIFI",
	         "Task \"%s\" received: %s",
	         pcTaskGetName(NULL),
	         xIncomingPublishCallbackContext.pcIncomingPublish);
}


void WIFI::MQTTpublish(int temp, int ah,int sh,int ws,int rn, int b1, int b2) {
	
	char pcTopicName[] = "SAFECLIMB";
    char pcPayload[256]; 

    snprintf(pcPayload, sizeof(pcPayload),
             "{\"temperature\": %d, \"airHumidity\": %d, \"soilHumidity\": %d, \"windSpeed\": %d, \"rain\": %d, \"battery_main\": %d, \"battery_repeater\": %d}",
             temp, ah, sh,ws,rn,b1,b2);
             
    printf("Battery: %d\n", b2);
	
	prvPublishToTopic( (MQTTQoS_t)1,pcTopicName,pcPayload,xMqttEventGroup );
	
	//prvWaitForEvent( xMqttEventGroup, MQTT_INCOMING_PUBLISH_RECEIVED_BIT );

    ESP_LOGI( "WIFI",
              "Task \"%s\" received: %s",
              pcTaskGetName( NULL ),
              xIncomingPublishCallbackContext.pcIncomingPublish );
}

void WIFI::prvCoreMqttAgentEventHandler( void * pvHandlerArg,esp_event_base_t xEventBase,int32_t lEventId,void * pvEventData )
{
	( void ) pvHandlerArg;
    ( void ) xEventBase;
    ( void ) pvEventData;

    switch( lEventId )
    {
        case CORE_MQTT_AGENT_CONNECTED_EVENT:
            ESP_LOGI( "WIFI",
                      "coreMQTT-Agent connected." );
            xEventGroupSetBits( xNetworkEventGroup,
                                CORE_MQTT_AGENT_CONNECTED_BIT );
            break;

        case CORE_MQTT_AGENT_DISCONNECTED_EVENT:
            ESP_LOGI( "WIFI",
                      "coreMQTT-Agent disconnected. Preventing coreMQTT-Agent "
                      "commands from being enqueued." );
            xEventGroupClearBits( xNetworkEventGroup,
                                  CORE_MQTT_AGENT_CONNECTED_BIT );
            break;

        case CORE_MQTT_AGENT_OTA_STARTED_EVENT:
            ESP_LOGI( "WIFI",
                      "OTA started. Preventing coreMQTT-Agent commands from "
                      "being enqueued." );
            xEventGroupClearBits( xNetworkEventGroup,
                                  CORE_MQTT_AGENT_OTA_NOT_IN_PROGRESS_BIT );
            break;

        case CORE_MQTT_AGENT_OTA_STOPPED_EVENT:
            ESP_LOGI( "WIFI",
                      "OTA stopped. No longer preventing coreMQTT-Agent "
                      "commands from being enqueued." );
            xEventGroupSetBits( xNetworkEventGroup,
                                CORE_MQTT_AGENT_OTA_NOT_IN_PROGRESS_BIT );
            break;

        default:
            ESP_LOGE( "WIFI",
                      "coreMQTT-Agent event handler received unexpected event: %" PRIu32 "",
                      lEventId );
            break;
    }
}

void WIFI::prvPublishToTopic( MQTTQoS_t xQoS,char * pcTopicName,char * pcPayload,EventGroupHandle_t xMqttEventGroup )
{
    uint32_t ulPublishMessageId = 0;

    MQTTStatus_t xCommandAdded;
    EventBits_t xReceivedEvent = 0;

    MQTTPublishInfo_t xPublishInfo = { };

    MQTTAgentCommandContext_t xCommandContext = { };
    MQTTAgentCommandInfo_t xCommandParams = { };

    xTaskNotifyStateClear( NULL );

    /* Create a unique number for the publish that is about to be sent.
     * This number is used in the command context and is sent back to this task
     * as a notification in the callback that's executed upon receipt of the
     * publish from coreMQTT-Agent.
     * That way this task can match an acknowledgment to the message being sent.
     */
    xSemaphoreTake( xMessageIdSemaphore, portMAX_DELAY );
    {
        ++ulMessageId;
        ulPublishMessageId = ulMessageId;
    }
    xSemaphoreGive( xMessageIdSemaphore );

    /* Configure the publish operation. The topic name string must persist for
     * duration of publish! */
    xPublishInfo.qos = xQoS;
    xPublishInfo.pTopicName = pcTopicName;
    xPublishInfo.topicNameLength = ( uint16_t ) strlen( pcTopicName );
    xPublishInfo.pPayload = pcPayload;
    xPublishInfo.payloadLength = ( uint16_t ) strlen( pcPayload );

    /* Complete an application defined context associated with this publish
     * message.
     * This gets updated in the callback function so the variable must persist
     * until the callback executes. */
    xCommandContext.xMqttEventGroup = xMqttEventGroup;

    xCommandParams.blockTimeMs = 500;
    xCommandParams.cmdCompleteCallback = prvPublishCommandCallback;
    xCommandParams.pCmdCompleteCallbackContext = &xCommandContext;

    do
    {
        /* Wait for coreMQTT-Agent task to have working network connection and
         * not be performing an OTA update. */
        xEventGroupWaitBits( xNetworkEventGroup,
                             CORE_MQTT_AGENT_CONNECTED_BIT | CORE_MQTT_AGENT_OTA_NOT_IN_PROGRESS_BIT,
                             pdFALSE,
                             pdTRUE,
                             portMAX_DELAY );

        ESP_LOGI( "WIFI",
                  "Task \"%s\" sending publish request to coreMQTT-Agent with message \"%s\" on topic \"%s\" with ID %" PRIu32 ".",
                  pcTaskGetName( NULL ),
                  pcPayload,
                  pcTopicName,
                  ulPublishMessageId );

        xCommandAdded = MQTTAgent_Publish( &xGlobalMqttAgentContext,
                                           &xPublishInfo,
                                           &xCommandParams );

        if( xCommandAdded == MQTTSuccess )
        {
            /* For QoS 1 and 2, wait for the publish acknowledgment.  For QoS0,
             * wait for the publish to be sent. */
            ESP_LOGI( "WIFI",
                      "Task \"%s\" waiting for publish %" PRIu32 " to complete.",
                      pcTaskGetName( NULL ),
                      ulPublishMessageId );

            xReceivedEvent = prvWaitForEvent( xMqttEventGroup,
                                              MQTT_PUBLISH_COMMAND_COMPLETED_BIT );
        }
        else
        {
            ESP_LOGE( "WIFI",
                      "Failed to enqueue publish command. Error code=%s",
                      MQTT_Status_strerror( xCommandAdded ) );
        }

        /* Check all ways the status was passed back just for demonstration
         * purposes. */
        if( ( ( xReceivedEvent & MQTT_PUBLISH_COMMAND_COMPLETED_BIT ) == 0 ) ||
            ( xCommandContext.xReturnStatus != MQTTSuccess ) )
        {
            ESP_LOGW( "WIFI",
                      "Error or timed out waiting for ack for publish message %" PRIu32 ". Re-attempting publish.",
                      ulPublishMessageId );
        }
        else
        {
            ESP_LOGI( "WIFI",
                      "Publish %" PRIu32 " succeeded for task \"%s\".",
                      ulPublishMessageId,
                      pcTaskGetName( NULL ) );
        }
    } while( ( xReceivedEvent & MQTT_PUBLISH_COMMAND_COMPLETED_BIT ) == 0 ||
             ( xCommandContext.xReturnStatus != MQTTSuccess ) );
}

EventBits_t WIFI::prvWaitForEvent( EventGroupHandle_t xMqttEventGroup,EventBits_t uxBitsToWaitFor )
{
    EventBits_t xReturn;

    xReturn = xEventGroupWaitBits( xMqttEventGroup,
                                   uxBitsToWaitFor,
                                   pdTRUE, /* xClearOnExit. */
                                   pdTRUE, /* xWaitForAllBits. */
                                   portMAX_DELAY );
    return xReturn;
}

void WIFI::prvPublishCommandCallback( MQTTAgentCommandContext_t * pxCommandContext,MQTTAgentReturnInfo_t * pxReturnInfo )
{
    /* Store the result in the application defined context so the task that
     * initiated the publish can check the operation's status. */
    pxCommandContext->xReturnStatus = pxReturnInfo->returnCode;

    if( pxCommandContext->xMqttEventGroup != NULL )
    {
        xEventGroupSetBits( pxCommandContext->xMqttEventGroup,
                            MQTT_PUBLISH_COMMAND_COMPLETED_BIT );
    }
}

void WIFI::prvSubscribeToTopic( IncomingPublishCallbackContext_t * pxIncomingPublishCallbackContext,
                                 MQTTQoS_t xQoS,
                                 char * pcTopicFilter,
                                 EventGroupHandle_t xMqttEventGroup )
{
    uint32_t ulSubscribeMessageId;

    MQTTStatus_t xCommandAdded;
    EventBits_t xReceivedEvent = 0;

    MQTTAgentSubscribeArgs_t xSubscribeArgs = {};
    MQTTSubscribeInfo_t xSubscribeInfo = {};

    MQTTAgentCommandContext_t xCommandContext = {};
    MQTTAgentCommandInfo_t xCommandParams = {};

    xTaskNotifyStateClear( NULL );

    /* Create a unique number for the subscribe that is about to be sent.
     * This number is used in the command context and is sent back to this task
     * as a notification in the callback that's executed upon receipt of the
     * publish from coreMQTT-Agent.
     * That way this task can match an acknowledgment to the message being sent.
     */
    xSemaphoreTake( xMessageIdSemaphore, portMAX_DELAY );
    {
        ++ulMessageId;
        ulSubscribeMessageId = ulMessageId;
    }
    xSemaphoreGive( xMessageIdSemaphore );

    /* Configure the subscribe operation.  The topic string must persist for
     * duration of subscription! */
    xSubscribeInfo.qos = xQoS;
    xSubscribeInfo.pTopicFilter = pcTopicFilter;
    xSubscribeInfo.topicFilterLength = ( uint16_t ) strlen( pcTopicFilter );

    xSubscribeArgs.pSubscribeInfo = &xSubscribeInfo;
    xSubscribeArgs.numSubscriptions = 1;

    /* Complete an application defined context associated with this subscribe
     * message.
     * This gets updated in the callback function so the variable must persist
     * until the callback executes. */
    xCommandContext.xMqttEventGroup = xMqttEventGroup;
    xCommandContext.pxIncomingPublishCallbackContext = pxIncomingPublishCallbackContext;
    xCommandContext.pArgs = ( void * ) &xSubscribeArgs;

    xCommandParams.blockTimeMs = 500;
    xCommandParams.cmdCompleteCallback = prvSubscribeCommandCallback;
    xCommandParams.pCmdCompleteCallbackContext = &xCommandContext;

    do
    {
        /* Wait for coreMQTT-Agent task to have working network connection and
         * not be performing an OTA update. */
        xEventGroupWaitBits( xNetworkEventGroup,
                             CORE_MQTT_AGENT_CONNECTED_BIT | CORE_MQTT_AGENT_OTA_NOT_IN_PROGRESS_BIT,
                             pdFALSE,
                             pdTRUE,
                             portMAX_DELAY );

        ESP_LOGI( "WIFI",
                  "Task \"%s\" sending subscribe request to coreMQTT-Agent for topic filter: %s with id %" PRIu32 "",
                  pcTaskGetName( NULL ),
                  pcTopicFilter,
                  ulSubscribeMessageId );

        xCommandAdded = MQTTAgent_Subscribe( &xGlobalMqttAgentContext,
                                             &xSubscribeArgs,
                                             &xCommandParams );

        if( xCommandAdded == MQTTSuccess )
        {
            /* For QoS 1 and 2, wait for the subscription acknowledgment. For QoS0,
             * wait for the subscribe to be sent. */
            xReceivedEvent = prvWaitForEvent( xMqttEventGroup,
                                              MQTT_SUBSCRIBE_COMMAND_COMPLETED_BIT );
        }
        else
        {
            ESP_LOGE( "WIFI",
                      "Failed to enqueue subscribe command. Error code=%s",
                      MQTT_Status_strerror( xCommandAdded ) );
        }

        /* Check all ways the status was passed back just for demonstration
         * purposes. */
        if( ( ( xReceivedEvent & MQTT_SUBSCRIBE_COMMAND_COMPLETED_BIT ) == 0 ) ||
            ( xCommandContext.xReturnStatus != MQTTSuccess ) )
        {
            ESP_LOGW( "WIFI",
                      "Error or timed out waiting for ack to subscribe message %" PRIu32 ". Re-attempting subscribe.",
                      ulSubscribeMessageId );
        }
        else
        {
            ESP_LOGI( "WIFI",
                      "Subscribe %" PRIu32 " for topic filter %s succeeded for task \"%s\".",
                      ulSubscribeMessageId,
                      pcTopicFilter,
                      pcTaskGetName( NULL ) );
        }
    } while( ( ( xReceivedEvent & MQTT_SUBSCRIBE_COMMAND_COMPLETED_BIT ) == 0 ) ||
             ( xCommandContext.xReturnStatus != MQTTSuccess ) );
}

void WIFI::prvSubscribeCommandCallback( MQTTAgentCommandContext_t * pxCommandContext,MQTTAgentReturnInfo_t * pxReturnInfo )
{
    bool xSubscriptionAdded = false;
    MQTTAgentSubscribeArgs_t * pxSubscribeArgs = ( MQTTAgentSubscribeArgs_t * ) pxCommandContext->pArgs;

    /* Store the result in the application defined context so the task that
     * initiated the subscribe can check the operation's status. */
    pxCommandContext->xReturnStatus = pxReturnInfo->returnCode;

    /* Check if the subscribe operation is a success. */
    if( pxReturnInfo->returnCode == MQTTSuccess )
    {
        /* Add subscription so that incoming publishes are routed to the application
         * callback. */
        xSubscriptionAdded = addSubscription( ( SubscriptionElement_t * ) xGlobalMqttAgentContext.pIncomingCallbackContext,
                                              pxSubscribeArgs->pSubscribeInfo->pTopicFilter,
                                              pxSubscribeArgs->pSubscribeInfo->topicFilterLength,
                                              prvIncomingPublishCallback,
                                              ( void * ) ( pxCommandContext->pxIncomingPublishCallbackContext ) );

        if( xSubscriptionAdded == false )
        {
            ESP_LOGE( "WIFI",
                      "Failed to register an incoming publish callback for topic %.*s.",
                      pxSubscribeArgs->pSubscribeInfo->topicFilterLength,
                      pxSubscribeArgs->pSubscribeInfo->pTopicFilter );
        }
    }

    if( pxCommandContext->xMqttEventGroup != NULL )
    {
        xEventGroupSetBits( pxCommandContext->xMqttEventGroup,
                            MQTT_SUBSCRIBE_COMMAND_COMPLETED_BIT );
    }
}

void WIFI::prvIncomingPublishCallback( void * pvIncomingPublishCallbackContext,
                                        MQTTPublishInfo_t * pxPublishInfo )
{
    IncomingPublishCallbackContext_t * pxIncomingPublishCallbackContext = ( IncomingPublishCallbackContext_t * ) pvIncomingPublishCallbackContext;

    size_t payloadLength = pxPublishInfo->payloadLength;

    if (payloadLength < sizeof(pxIncomingPublishCallbackContext->pcIncomingPublish)) {
        memcpy(pxIncomingPublishCallbackContext->pcIncomingPublish, pxPublishInfo->pPayload, payloadLength);
    } else {
        ESP_LOGW("MQTT", "Mensagem maior que o buffer, cortando para %d bytes", sizeof(pxIncomingPublishCallbackContext->pcIncomingPublish));
        memcpy(pxIncomingPublishCallbackContext->pcIncomingPublish, pxPublishInfo->pPayload, sizeof(pxIncomingPublishCallbackContext->pcIncomingPublish));
    }

    pxIncomingPublishCallbackContext->pcIncomingPublish[ payloadLength < sizeof(pxIncomingPublishCallbackContext->pcIncomingPublish) ? payloadLength : sizeof(pxIncomingPublishCallbackContext->pcIncomingPublish) - 1 ] = 0x00;
    xEventGroupSetBits( pxIncomingPublishCallbackContext->xMqttEventGroup, MQTT_INCOMING_PUBLISH_RECEIVED_BIT );
}

void WIFI::MQTTreceive()
{
	prvWaitForEvent( xMqttEventGroup, MQTT_INCOMING_PUBLISH_RECEIVED_BIT );
	
	size_t totallenght = strlen((const char*)xIncomingPublishCallbackContext.pcIncomingPublish);
	printf("O tamanho é de %d",totallenght);

    ESP_LOGI( "WIFI",
              "Task \"%s\" received: %s",
              pcTaskGetName( NULL ),
              xIncomingPublishCallbackContext.pcIncomingPublish );
              
    uint8_t* message = extractMessage((const uint8_t* )xIncomingPublishCallbackContext.pcIncomingPublish);
    size_t messageLength = strlen((const char*)message);
    printf("MENSAGEM RECEBIDA POR MQTT: %s",(char*)message);
	if (messageLength > LORA_BYTES)
	{
	    size_t ind = 0;
	    while (ind < messageLength)
	    {
	        size_t diff = (messageLength - ind) > LORA_BYTES ? LORA_BYTES : (messageLength - ind);
	        uint8_t bufSec[LORA_BYTES]; 
	        memcpy(bufSec, message + ind, diff);
	        if(diff < LORA_BYTES)
	        	lora->sendPackage(bufSec, diff, lora->getBluetoothTower(), 3, true);
	        else
 				lora->sendPackage(bufSec, diff, lora->getBluetoothTower(), 1, true);
	        ind += diff; 
	    }
	}
	else
	{
	    lora->sendPackage(message, messageLength, lora->getBluetoothTower(),3,true);
	}
    
    //lora->setHigherOrder(true);
}

void WIFI::MQTTsubscribe()
{
	char pcTopicName[] = "SOS/IN";
    prvSubscribeToTopic( &xIncomingPublishCallbackContext,
                             (MQTTQoS_t)1,
                             pcTopicName,
                             xMqttEventGroup );
}

void WIFI::setLoraManager(LoraManager* l)
{
	lora = l;
}

uint8_t* WIFI::extractMessage(const uint8_t* jsonInput) {
    const char* jsonString = reinterpret_cast<const char*>(jsonInput);
    const char* key = "\"Message\": ";
    const char* keyPosition = strstr(jsonString, key);

    if (!keyPosition) {
        return nullptr;
    }

    const char* valueStart = keyPosition + strlen(key);

    if (*valueStart != '\"') {
        return nullptr;
    }
    valueStart++;

    const char* valueEnd = strchr(valueStart, '\"');
    if (!valueEnd) {
        return nullptr;
    }

    size_t maxLength = valueEnd - valueStart;
    uint8_t* result = new uint8_t[maxLength * 4 + 1]; 

    size_t resultIndex = 0;
    const char* input = valueStart;

    while (input < valueEnd) {
        if (*input == '\\' && *(input + 1) == 'u') {
            // Decodificar \uXXXX
            char hex[5] = {input[2], input[3], input[4], input[5], '\0'};
            uint16_t codepoint = (uint16_t)strtol(hex, nullptr, 16);

            // Converter para UTF-8
            if (codepoint <= 0x7F) {
                result[resultIndex++] = (uint8_t)codepoint;
            } else if (codepoint <= 0x7FF) {
                result[resultIndex++] = (uint8_t)(0xC0 | (codepoint >> 6));
                result[resultIndex++] = (uint8_t)(0x80 | (codepoint & 0x3F));
            } else {
                result[resultIndex++] = (uint8_t)(0xE0 | (codepoint >> 12));
                result[resultIndex++] = (uint8_t)(0x80 | ((codepoint >> 6) & 0x3F));
                result[resultIndex++] = (uint8_t)(0x80 | (codepoint & 0x3F));
            }

            input += 6;
        } else {
        
            result[resultIndex++] = (uint8_t)(*input);
            input++;
        }
    }

    result[resultIndex] = '\0'; 
    return result;
}

void WIFI::setHelloMessage(bool hello)
{
	sendHelloMessage = true;
}
