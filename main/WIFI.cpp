
#include "WIFI.h"


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
	
}

WIFI::~WIFI() 
{

}

void WIFI::init()
{
	ESP_LOGI("WIFI","Inicializando wifi");
	
	esp_err_t xEspErrRet;
		
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

void WIFI::MQTTpublish(int temp, int ah,int sh,int ws,int rn) {
	
	char pcTopicName[] = "SAFECLIMB";
    char pcPayload[128]; 

    snprintf(pcPayload, sizeof(pcPayload),
             "{\"temperature\": %d, \"airHumidity\": %d, \"soilHumidity\": %d, \"windSpeed\": %d, \"rain\": %d}",
             temp, ah, sh,ws,rn);
	
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
