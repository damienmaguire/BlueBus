/*
 * File: handler.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and IBus communicate
 */
#include "handler.h"
static HandlerContext_t Context;
static char *PROFILES[] = {
    "A2DP",
    "AVRCP",
    "",
    "HFP",
    "BLE",
    "",
    "",
    "",
    "MAP"
};

/**
 * HandlerInit()
 *     Description:
 *         Initialize our context and register all event listeners and
 *         scheduled tasks.
 *     Params:
 *         BC127_t *bt - The BC127 Object
 *         IBus_t *ibus - The IBus Object
 *     Returns:
 *         void
 */
void HandlerInit(BC127_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    uint32_t now = TimerGetMillis();
    Context.btDeviceConnRetries = 0;
    Context.btStartupIsRun = 0;
    Context.btConnectionStatus = HANDLER_BT_CONN_OFF;
    Context.btSelectedDevice = HANDLER_BT_SELECTED_DEVICE_NONE;
    Context.uiMode = ConfigGetUIMode();
    Context.seekMode = HANDLER_CDC_SEEK_MODE_NONE;
    Context.lmDimmerChecksum = 0x00;
    Context.mflButtonStatus = HANDLER_MFL_STATUS_OFF;
    Context.telStatus = IBUS_TEL_STATUS_ACTIVE_POWER_HANDSFREE;
    Context.btBootFailure = HANDLER_BT_BOOT_OK;
    memset(&Context.bodyModuleStatus, 0, sizeof(HandlerBodyModuleStatus_t));
    memset(&Context.lightControlStatus, 0, sizeof(HandlerLightControlStatus_t));
    memset(&Context.ibusModuleStatus, 0, sizeof(HandlerModuleStatus_t));
    Context.powerStatus = HANDLER_POWER_ON;
    Context.scanIntervals = 0;
    Context.lmLastIOStatus = now;
    Context.cdChangerLastPoll = now;
    Context.cdChangerLastStatus = now;
    EventRegisterCallback(
        BC127Event_Boot,
        &HandlerBC127Boot,
        &Context
    );
    EventRegisterCallback(
        BC127Event_BootStatus,
        &HandlerBC127BootStatus,
        &Context
    );
    EventRegisterCallback(
        BC127Event_CallStatus,
        &HandlerBC127CallStatus,
        &Context
    );
    EventRegisterCallback(
        BC127Event_CallerID,
        &HandlerBC127CallerID,
        &Context
    );
    EventRegisterCallback(
        BC127Event_DeviceLinkConnected,
        &HandlerBC127DeviceLinkConnected,
        &Context
    );
    EventRegisterCallback(
        BC127Event_DeviceDisconnected,
        &HandlerBC127DeviceDisconnected,
        &Context
    );
    EventRegisterCallback(
        BC127Event_DeviceFound,
        &HandlerBC127DeviceFound,
        &Context
    );
    EventRegisterCallback(
        BC127Event_PlaybackStatusChange,
        &HandlerBC127PlaybackStatus,
        &Context
    );
    EventRegisterCallback(
        UIEvent_CloseConnection,
        &HandlerUICloseConnection,
        &Context
    );
    EventRegisterCallback(
        UIEvent_InitiateConnection,
        &HandlerUIInitiateConnection,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_CDStatusRequest,
        &HandlerIBusCDCStatus,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_DSPConfigSet,
        &HandlerIBusDSPConfigSet,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_FirstMessageReceived,
        &HandlerIBusFirstMessageReceived,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_DoorsFlapsStatusResponse,
        &HandlerIBusGMDoorsFlapsStatusResponse,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_GTDIAIdentityResponse,
        &HandlerIBusGTDIAIdentityResponse,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_GTDIAOSIdentityResponse,
        &HandlerIBusGTDIAOSIdentityResponse,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_IKEIgnitionStatus,
        &HandlerIBusIKEIgnitionStatus,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_IKESpeedRPMUpdate,
        &HandlerIBusIKESpeedRPMUpdate,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_IKEVehicleType,
        &HandlerIBusIKEVehicleType,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_LCMLightStatus,
        &HandlerIBusLMLightStatus,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_LCMDimmerStatus,
        &HandlerIBusLMDimmerStatus,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_LCMRedundantData,
        &HandlerIBusLMRedundantData,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_LMIdentResponse,
        &HandlerIBusLMIdentResponse,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_MFLButton,
        &HandlerIBusMFLButton,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_ModuleStatusRequest,
        &HandlerIBusModuleStatusRequest,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_ModuleStatusResponse,
        &HandlerIBusModuleStatusResponse,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_RADVolumeChange,
        &HandlerIBusRADVolumeChange,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_TELVolumeChange,
        &HandlerIBusTELVolumeChange,
        &Context
    );
    TimerRegisterScheduledTask(
        &HandlerTimerBC127State,
        &Context,
        HANDLER_INT_BC127_STATE
    );
    TimerRegisterScheduledTask(
        &HandlerTimerCDCAnnounce,
        &Context,
        HANDLER_INT_CDC_ANOUNCE
    );
    TimerRegisterScheduledTask(
        &HandlerTimerCDCSendStatus,
        &Context,
        HANDLER_INT_CDC_STATUS
    );
    TimerRegisterScheduledTask(
        &HandlerTimerDeviceConnection,
        &Context,
        HANDLER_INT_DEVICE_CONN
    );
    TimerRegisterScheduledTask(
        &HandlerTimerLCMIOStatus,
        &Context,
        HANDLER_INT_LCM_IO_STATUS
    );
    TimerRegisterScheduledTask(
        &HandlerTimerOpenProfileErrors,
        &Context,
        HANDLER_INT_PROFILE_ERROR
    );
    TimerRegisterScheduledTask(
        &HandlerTimerPoweroff,
        &Context,
        HANDLER_INT_POWEROFF
    );
    TimerRegisterScheduledTask(
        &HandlerTimerScanDevices,
        &Context,
        HANDLER_INT_DEVICE_SCAN
    );

    BC127CommandStatus(Context.bt);
    if (Context.uiMode == IBus_UI_CD53 ||
        Context.uiMode == IBus_UI_BUSINESS_NAV
    ) {
        CD53Init(bt, ibus);
    } else if (Context.uiMode == IBus_UI_BMBT) {
        BMBTInit(bt, ibus);
    } else if (Context.uiMode == IBus_UI_MID) {
        MIDInit(bt, ibus);
    } else if (Context.uiMode == IBus_UI_MID_BMBT) {
        MIDInit(bt, ibus);
        BMBTInit(bt, ibus);
    }
    if (ConfigGetSetting(CONFIG_SETTING_IGN_ALWAYS_ON) == CONFIG_SETTING_ON) {
        unsigned char ignitionStatus = 0x01;
        EventTriggerCallback(
            IBUS_EVENT_IKEIgnitionStatus,
            (unsigned char *)&ignitionStatus
        );
        ibus->ignitionStatus = ignitionStatus;
    }
}

static void HandlerSwitchUI(HandlerContext_t *context, unsigned char newUi)
{
    // Unregister the previous UI
    if (context->uiMode == IBus_UI_CD53 ||
        context->uiMode == IBus_UI_BUSINESS_NAV
    ) {
        CD53Destroy();
    } else if (context->uiMode == IBus_UI_BMBT) {
        BMBTDestroy();
    } else if (context->uiMode == IBus_UI_MID) {
        MIDDestroy();
    } else if (context->uiMode == IBus_UI_MID_BMBT) {
        MIDDestroy();
        BMBTDestroy();
    }
    if (newUi == IBus_UI_CD53 || newUi == IBus_UI_BUSINESS_NAV) {
        CD53Init(context->bt, context->ibus);
    } else if (newUi == IBus_UI_BMBT) {
        BMBTInit(context->bt, context->ibus);
    } else if (newUi == IBus_UI_MID) {
        MIDInit(context->bt, context->ibus);
    } else if (newUi == IBus_UI_MID_BMBT) {
        MIDInit(context->bt, context->ibus);
        BMBTInit(context->bt, context->ibus);
    }
    ConfigSetUIMode(newUi);
    context->uiMode = newUi;
}

/**
 * HandlerBC127Boot()
 *     Description:
 *         If the BC127 restarts, reset our internal state
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127Boot(void *ctx, unsigned char *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BC127ClearPairedDevices(context->bt);
    BC127CommandStatus(context->bt);
}

/**
 * HandlerBC127BootStatus()
 *     Description:
 *         If the BC127 Radios are off, meaning we rebooted and got the status
 *         back, then alter the module status to match the ignition status
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127BootStatus(void *ctx, unsigned char *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    context->btConnectionStatus = HANDLER_BT_CONN_OFF;
    BC127CommandList(context->bt);
    if (context->ibus->ignitionStatus == IBUS_IGNITION_OFF) {
        // Set the BT module not connectable or discoverable and disconnect all devices
        BC127CommandBtState(context->bt, BC127_STATE_OFF, BC127_STATE_OFF);
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
    } else {
        // Set the connectable and discoverable states to what they were
        BC127CommandBtState(context->bt, BC127_STATE_ON, context->bt->discoverable);
    }
}

/**
 * HandlerBC127CallStatus()
 *     Description:
 *         Handle call status updates
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127CallStatus(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // If we were playing before the call, try to resume playback
    if (context->bt->callStatus == BC127_CALL_INACTIVE &&
        context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING
    ) {
        BC127CommandPlay(context->bt);
    }
    // Tell the vehicle what the call status is
    uint8_t statusChange = HandlerIBusBroadcastTELStatus(
        context,
        HANDLER_TEL_STATUS_SET
    );
    if (statusChange == 1) {
        LogDebug(LOG_SOURCE_SYSTEM, "Call > TCU");
        // Handle volume control
        if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING &&
            (
                ConfigGetSetting(CONFIG_SETTING_USE_SPDIF_INPUT) == CONFIG_SETTING_OFF ||
                context->ibusModuleStatus.DSP == 0
            )
        ) {
            if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE) {
                LogDebug(LOG_SOURCE_SYSTEM, "Call > TCU > Begin");
                // Enable the amp and mute the radio
                PAM_SHDN = 1;
                TEL_MUTE = 1;
                // Set the DAC Volume to the "telephone" volume
                PCM51XXSetVolume(ConfigGetSetting(CONFIG_SETTING_DAC_TEL_TCU_MODE_VOL));
            } else {
                LogDebug(LOG_SOURCE_SYSTEM, "Call > TCU > End");
                // Reset the DAC volume
                PCM51XXSetVolume(ConfigGetSetting(CONFIG_SETTING_DAC_AUDIO_VOL));
                // Disable the amp and unmute the radio
                PAM_SHDN = 0;
                TimerDelayMicroseconds(250);
                TEL_MUTE = 0;
            }
        } else {
            unsigned char volume = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
            if (context->uiMode == IBus_UI_BMBT ||
                context->uiMode == IBus_UI_MID ||
                context->uiMode == IBus_UI_MID_BMBT
            ) {
                LogDebug(LOG_SOURCE_SYSTEM, "Call > NAV_MID");
                if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE) {
                    if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING) {
                        IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
                    }
                    LogDebug(LOG_SOURCE_SYSTEM, "Call > NAV_MID > Begin");
                    if (strlen(context->bt->callerId) > 0) {
                        IBusCommandTELStatusText(context->ibus, context->bt->callerId, 0);
                    }
                    if (volume > HANDLER_TEL_VOL_OFFSET_MAX) {
                        volume = HANDLER_TEL_VOL_OFFSET_MAX;
                        ConfigSetSetting(CONFIG_SETTING_TEL_VOL, HANDLER_TEL_VOL_OFFSET_MAX);
                    }
                    LogDebug(LOG_SOURCE_SYSTEM, "Call > NAV_MID > Volume: %d", volume);
                    unsigned char sourceSystem = IBUS_DEVICE_BMBT;
                    if (context->ibusModuleStatus.MID == 1) {
                        sourceSystem = IBUS_DEVICE_MID;
                    }
                    while (volume > 0) {
                        unsigned char volStep = volume;
                        if (volStep > 0x03) {
                            volStep = 0x03;
                        }
                        IBusCommandSetVolume(
                            context->ibus,
                            sourceSystem,
                            IBUS_DEVICE_RAD,
                            (volStep << 4) | 0x01
                        );
                        volume = volume - volStep;
                    }
                } else {
                    LogDebug(LOG_SOURCE_SYSTEM, "Call > NAV_MID > End");
                    // Reset the NAV / DSP / MID volume
                    // Temporarily set the call status flag to on so we do not alter
                    // the volume we are lowering ourselves
                    context->telStatus = HANDLER_TEL_STATUS_VOL_CHANGE;
                    while (volume > 0) {
                        unsigned char sourceSystem = IBUS_DEVICE_BMBT;
                        if (context->ibusModuleStatus.MID == 1) {
                            sourceSystem = IBUS_DEVICE_MID;
                        }
                        unsigned char volStep = volume;
                        if (volStep > 0x03) {
                            volStep = 0x03;
                        }
                        IBusCommandSetVolume(
                            context->ibus,
                            sourceSystem,
                            IBUS_DEVICE_RAD,
                            volStep << 4
                        );
                        volume = volume - volStep;
                    }
                    context->telStatus = IBUS_TEL_STATUS_ACTIVE_POWER_HANDSFREE;
                    if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING) {
                        IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
                    }
                }
            } else {
                LogDebug(LOG_SOURCE_SYSTEM, "Call > CD53 > Begin");
            }
        }
    }
}

/**
 * HandlerBC127CallStatus()
 *     Description:
 *         Handle caller ID updates
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127CallerID(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE) {
        LogDebug(LOG_SOURCE_SYSTEM, "Call > ID: %s", context->bt->callerId);
        IBusCommandTELStatusText(context->ibus, context->bt->callerId, 0);
    }
}

/**
 * HandlerBC127DeviceLinkConnected()
 *     Description:
 *         If a device link is opened, disable connectability once all profiles
 *         are opened. Otherwise if the ignition is off, disconnect all devices
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *data - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127DeviceLinkConnected(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->ibus->ignitionStatus > IBUS_IGNITION_OFF) {
        // Once A2DP and AVRCP are connected, we can disable connectability
        // If HFP is enabled, do not disable connectability until the
        // profile opens
        if (context->bt->activeDevice.avrcpLinkId != 0 &&
            context->bt->activeDevice.a2dpLinkId != 0
        ) {
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_OFF ||
                context->bt->activeDevice.hfpLinkId != 0
            ) {
                LogDebug(LOG_SOURCE_SYSTEM, "Handler: Disable connect-ability");
                BC127CommandBtState(
                    context->bt,
                    BC127_STATE_OFF,
                    context->bt->discoverable
                );
                if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == CONFIG_SETTING_ON &&
                    context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING
                ) {
                    BC127CommandPlay(context->bt);
                }
            } else if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON &&
                       context->bt->activeDevice.hfpLinkId == 0
            ) {
                char *macId = (char *) context->bt->activeDevice.macId;
                BC127CommandProfileOpen(context->bt, macId, "HFP");
            }
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
                IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_GREEN);
            }
        }
    } else {
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
    }
}

/**
 * HandlerBC127DeviceDisconnected()
 *     Description:
 *         If a device disconnects and our ignition is on,
 *         make the module connectable again.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127DeviceDisconnected(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // Reset the metadata so we don't display the wrong data
    BC127ClearMetadata(context->bt);
    BC127ClearPairingErrors(context->bt);
    if (context->ibus->ignitionStatus > IBUS_IGNITION_OFF) {
        BC127CommandBtState(
            context->bt,
            BC127_STATE_ON,
            context->bt->discoverable
        );
        if (context->btConnectionStatus == HANDLER_BT_CONN_CHANGE) {
            BC127PairedDevice_t *dev = &context->bt->pairedDevices[
                context->btSelectedDevice
            ];
            BC127CommandProfileOpen(context->bt, dev->macId, "A2DP");
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
                BC127CommandProfileOpen(context->bt, dev->macId, "HFP");
            }
            context->btSelectedDevice = HANDLER_BT_SELECTED_DEVICE_NONE;
        } else {
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
                IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_RED);
            }
            BC127CommandList(context->bt);
        }
    }
    context->btConnectionStatus = HANDLER_BT_CONN_OFF;
}

/**
 * HandlerBC127DeviceFound()
 *     Description:
 *         If a device is found and we are not connected, connect to it
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127DeviceFound(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->activeDevice.deviceId == 0 &&
        context->btConnectionStatus == HANDLER_BT_CONN_OFF &&
        context->ibus->ignitionStatus > IBUS_IGNITION_OFF
    ) {
        char *macId = (char *) data;
        LogDebug(LOG_SOURCE_SYSTEM, "Handler: No Device -- Attempt connection");
        BC127CommandProfileOpen(context->bt, macId, "A2DP");
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
            BC127CommandProfileOpen(context->bt, macId, "HFP");
        }
        context->btConnectionStatus = HANDLER_BT_CONN_ON;
    } else {
        LogDebug(
            LOG_SOURCE_SYSTEM,
            "Handler: Not connecting to new device %d %d %d",
            context->bt->activeDevice.deviceId,
            context->btConnectionStatus,
            context->ibus->ignitionStatus
        );
    }
}

/**
 * HandlerBC127PlaybackStatus()
 *     Description:
 *         If the application is starting, request the BC127 AVRCP Metadata
 *         if it is playing. If the CD Change status is not set to "playing"
 *         then we pause playback.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127PlaybackStatus(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // If this is the first status update
    if (context->btStartupIsRun == 0) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            // Request Metadata
            BC127CommandGetMetadata(context->bt);
        }
        context->btStartupIsRun = 1;
    }
    if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING &&
        context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING
    ) {
        // We're playing but not in Bluetooth mode - stop playback
        BC127CommandPause(context->bt);
    }
}

/**
 * HandlerUICloseConnection()
 *     Description:
 *         Close the active connection and dissociate ourselves from it
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerUICloseConnection(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // Reset the metadata so we don't display the wrong data
    BC127ClearMetadata(context->bt);
    // Clear the actively paired device
    BC127ClearActiveDevice(context->bt);
    // Close All connections
    BC127CommandClose(context->bt, BC127_CLOSE_ALL);
    // Enable connectivity
    BC127CommandBtState(context->bt, BC127_STATE_ON, context->bt->discoverable);

}

/**
 * HandlerUIInitiateConnection()
 *     Description:
 *         Handle the connection when a new device is selected in the UI
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerUIInitiateConnection(void *ctx, unsigned char *deviceId)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->activeDevice.deviceId != 0) {
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
    }
    context->btSelectedDevice = (int8_t) *deviceId;
    context->btConnectionStatus = HANDLER_BT_CONN_CHANGE;
}

/**
 * HandlerIBusCDCStatus()
 *     Description:
 *         Track the current CD Changer status based on what the radio
 *         instructs us to do. We respond with exactly what the radio instructs
 *         even if we haven't done it yet. Otherwise, the radio will continue
 *         to accost us to do what it wants
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusCDCStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char curStatus = IBUS_CDC_STAT_STOP;
    unsigned char curFunction = IBUS_CDC_FUNC_NOT_PLAYING;
    unsigned char requestedCommand = pkt[4];
    if (requestedCommand == IBUS_CDC_CMD_GET_STATUS) {
        curFunction = context->ibus->cdChangerFunction;
        if (curFunction == IBUS_CDC_FUNC_PLAYING) {
            curStatus = IBUS_CDC_STAT_PLAYING;
        } else if (curFunction == IBUS_CDC_FUNC_PAUSE) {
            curStatus = IBUS_CDC_STAT_PAUSE;
        }
    } else if (requestedCommand == IBUS_CDC_CMD_STOP_PLAYING) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            BC127CommandPause(context->bt);
        }
        curStatus = IBUS_CDC_STAT_STOP;
        curFunction = IBUS_CDC_FUNC_NOT_PLAYING;
        // Return to non-S/PDIF input once told to stop playback, if enabled
        if (ConfigGetSetting(CONFIG_SETTING_USE_SPDIF_INPUT) == CONFIG_SETTING_ON) {
            IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
        }
    } else if (requestedCommand == IBUS_CDC_CMD_CHANGE_TRACK ||
               requestedCommand == IBUS_CDC_CMD_CHANGE_TRACK_BLAUPUNKT
    ) {
        curFunction = context->ibus->cdChangerFunction;
        curStatus = IBUS_CDC_STAT_PLAYING;
        // Do not go backwards/forwards if the UI is CD53 because
        // those actions can be used to use the UI
        if (context->uiMode != IBus_UI_CD53) {
            if (pkt[5] == 0x00) {
                BC127CommandForward(context->bt);
            } else {
                BC127CommandBackward(context->bt);
            }
        }
    } else if (requestedCommand == IBUS_CDC_CMD_SEEK) {
        if (pkt[5] == 0x00) {
            context->seekMode = HANDLER_CDC_SEEK_MODE_REV;
            BC127CommandBackwardSeekPress(context->bt);
        } else {
            context->seekMode = HANDLER_CDC_SEEK_MODE_FWD;
            BC127CommandForwardSeekPress(context->bt);
        }
        curStatus = IBUS_CDC_STAT_FAST_REV;
    } else if (requestedCommand == IBUS_CDC_CMD_CD_CHANGE) {
        curStatus = IBUS_CDC_STAT_PLAYING;
        curFunction = IBUS_CDC_FUNC_PLAYING;
    } else if (requestedCommand == IBUS_CDC_CMD_SCAN) {
        curStatus = 0x00;
        // The 5th octet in the packet tells the CDC if we should
        // enable or disable the given mode
        if (pkt[5] == 0x01) {
            curFunction = IBUS_CDC_FUNC_SCAN_MODE;
        } else {
            curFunction = IBUS_CDC_FUNC_PLAYING;
        }
    } else if (requestedCommand == IBUS_CDC_CMD_RANDOM_MODE) {
        curStatus = 0x00;
        // The 5th octet in the packet tells the CDC if we should
        // enable or disable the given mode
        if (pkt[5] == 0x01) {
            curFunction = IBUS_CDC_FUNC_RANDOM_MODE;
        } else {
            curFunction = IBUS_CDC_FUNC_PLAYING;
        }
    } else {
        if (requestedCommand == IBUS_CDC_CMD_PAUSE_PLAYING) {
            curStatus = IBUS_CDC_STAT_PAUSE;
            curFunction = IBUS_CDC_FUNC_PAUSE;
        } else if (requestedCommand == IBUS_CDC_CMD_START_PLAYING) {
            curStatus = IBUS_CDC_STAT_PLAYING;
            curFunction = IBUS_CDC_FUNC_PLAYING;
            if (context->seekMode == HANDLER_CDC_SEEK_MODE_FWD) {
                BC127CommandForwardSeekRelease(context->bt);
                context->seekMode = HANDLER_CDC_SEEK_MODE_NONE;
            } else if (context->seekMode == HANDLER_CDC_SEEK_MODE_REV) {
                BC127CommandBackwardSeekRelease(context->bt);
                context->seekMode = HANDLER_CDC_SEEK_MODE_NONE;
            }
            // Set the Input to S/PDIF once told to start playback, if enabled
            if (ConfigGetSetting(CONFIG_SETTING_USE_SPDIF_INPUT) == CONFIG_SETTING_ON) {
                IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
            }
            // UI Identification
            if (ConfigGetUIMode() == 0 && TimerGetMillis() > 60000) {
                if (context->ibusModuleStatus.MID == 0 &&
                    context->ibusModuleStatus.GT == 0
                ) {
                    // Fallback for vehicle UI Identification
                    // If no UI has been detected and we have been
                    // running at least a minute, default to CD53 UI
                    LogInfo(LOG_SOURCE_SYSTEM, "Fallback to CD53 UI");
                    HandlerSwitchUI(context, IBus_UI_CD53);
                } else if (context->ibusModuleStatus.GT == 1) {
                    // Request the Navigation Identity
                    IBusCommandDIAGetIdentity(context->ibus, IBUS_DEVICE_GT);
                    IBusCommandDIAGetOSIdentity(context->ibus, IBUS_DEVICE_GT);
                }
            }
        } else {
            curStatus = requestedCommand;
        }
    }
    unsigned char discCount = IBUS_CDC_DISC_COUNT_6;
    // Report disc 7 loaded so any button press causes a CD Changer command
    // to be sent by the RAD (since there is no 7th disc)
    unsigned char discNumber = 0x07;
    if (context->uiMode == IBus_UI_BMBT) {
        discCount = IBUS_CDC_DISC_COUNT_1;
        discNumber = 0x01;
    }
    IBusCommandCDCStatus(
        context->ibus,
        curStatus,
        curFunction,
        discCount,
        discNumber
    );
    context->cdChangerLastPoll = TimerGetMillis();
    context->cdChangerLastStatus = TimerGetMillis();
}

/**
 * HandlerIBusDSPConfigSet()
 *     Description:
 *         Sort through received DSP configurations to ensure
 *         the correct input source is selected when appropriate
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusDSPConfigSet(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (pkt[4] == IBUS_DSP_CONFIG_SET_INPUT_RADIO &&
        context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING &&
        ConfigGetSetting(CONFIG_SETTING_USE_SPDIF_INPUT) == CONFIG_SETTING_ON
    ) {
        // Set the Input to S/PDIF if we are overridden
        IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
    }
}

/**
 * HandlerIBusFirstMessageReceived()
 *     Description:
 *         Request module status after the first IBus message is received.
 *         DO NOT change the order in which these modules are polled.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusFirstMessageReceived(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    IBusCommandGetModuleStatus(
        context->ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_IKE
    );
    IBusCommandGetModuleStatus(
        context->ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_GT
    );
    IBusCommandGetModuleStatus(
        context->ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_MID
    );
    IBusCommandGetModuleStatus(
        context->ibus,
        IBUS_DEVICE_CDC,
        IBUS_DEVICE_RAD
    );
    IBusCommandGetModuleStatus(
        context->ibus,
        IBUS_DEVICE_IKE,
        IBUS_DEVICE_LCM
    );
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
        IBusCommandSetModuleStatus(
            context->ibus,
            IBUS_DEVICE_TEL,
            IBUS_DEVICE_LOC,
            IBUS_TEL_SIG_EVEREST | 0x01
        );
    }
    IBusCommandIKEGetIgnitionStatus(context->ibus);
}

/**
 * HandlerIBusGMDoorsFlapStatusResponse()
 *     Description:
 *         Track which doors have been opened while the ignition was on
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *type - The navigation type
 *     Returns:
 *         void
 */
void HandlerIBusGMDoorsFlapsStatusResponse(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bodyModuleStatus.lowSideDoors == 0) {
        unsigned char doorStatus = pkt[4] & 0x0F;
        if (doorStatus > 0x01) {
            context->bodyModuleStatus.lowSideDoors = 1;
        }
    }
    // The 5th bit in the first data byte contains the lock status
    if (CHECK_BIT(pkt[4], 5) != 0) {
        LogInfo(LOG_SOURCE_SYSTEM, "Handler: Central Locks locked");
        context->bodyModuleStatus.doorsLocked = 1;
    } else {
        LogInfo(LOG_SOURCE_SYSTEM, "Handler: Central Locks unlocked");
        context->bodyModuleStatus.doorsLocked = 0;
    }
}

/**
 * HandlerIBusGTDIAIdentityResponse()
 *     Description:
 *         Identify the navigation module hardware and software versions
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *type - The navigation type
 *     Returns:
 *         void
 */
void HandlerIBusGTDIAIdentityResponse(void *ctx, unsigned char *type)
{
    unsigned char navType = *type;
    if (ConfigGetNavType() != navType) {
        ConfigSetNavType(navType);
    }
}

/**
 * HandlerIBusGTDIAOSIdentityResponse()
 *     Description:
 *         Identify the navigation module type from its OS
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - Data packet
 *     Returns:
 *         void
 */
void HandlerIBusGTDIAOSIdentityResponse(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    char navigationOS[8] = {};
    uint8_t i;
    for (i = 0; i < 7; i++) {
        navigationOS[i] = pkt[i + 4];
    }
    // The string should come null terminated, but we should not trust that
    navigationOS[7] = '\0';
    if (UtilsStricmp(navigationOS, "BMWC01S") == 0) {
        if (context->ibusModuleStatus.MID == 0) {
            if (ConfigGetUIMode() != IBus_UI_BMBT) {
                LogInfo(LOG_SOURCE_SYSTEM, "Detected BMBT UI");
                HandlerSwitchUI(context, IBus_UI_BMBT);
            }
        } else {
            if (ConfigGetUIMode() != IBus_UI_MID_BMBT) {
                LogInfo(LOG_SOURCE_SYSTEM, "Detected MID / BMBT UI");
                HandlerSwitchUI(context, IBus_UI_MID_BMBT);
            }
        }
    } else if (UtilsStricmp(navigationOS, "BMWM01S") == 0) {
        if (ConfigGetUIMode() != IBus_UI_BUSINESS_NAV) {
            LogInfo(LOG_SOURCE_SYSTEM, "Detected Business Nav UI");
            HandlerSwitchUI(context, IBus_UI_BUSINESS_NAV);
        }
    } else {
        LogError("Unable to identify GT OS: %s", navigationOS);
    }
}

/**
 * HandlerIBusIKEIgnitionStatus()
 *     Description:
 *         Track the Ignition state and update the BC127 accordingly. We set
 *         the BT device "off" when the key is set to position 0 and on
 *         as soon as it goes to a position >= 1.
 *         Request the LCM status when the car is turned to or past position 1
 *         Unlock the vehicle once the key is turned to position 1
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusIKEIgnitionStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char ignitionStatus = pkt[0];
    // If the ignition status has changed
    if (ignitionStatus != context->ibus->ignitionStatus) {
        // If the first bit is set, the key is in position 1 at least, otherwise
        // the ignition is off
        if (ignitionStatus == IBUS_IGNITION_OFF) {
            // Disable Telephone On
            TEL_ON = 0;
            // Set the BT module not connectable/discoverable. Disconnect all devices
            BC127CommandBtState(context->bt, BC127_STATE_OFF, BC127_STATE_OFF);
            BC127CommandClose(context->bt, BC127_CLOSE_ALL);
            BC127ClearPairedDevices(context->bt);
            // Unlock the vehicle
            if (ConfigGetComfortUnlock() == CONFIG_SETTING_COMFORT_UNLOCK_POS_0 &&
                context->bodyModuleStatus.doorsLocked == 1
            ) {
                if (context->ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
                    IBusCommandGMDoorCenterLockButton(context->ibus);
                } else if (context->ibus->vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
                    if (context->bodyModuleStatus.lowSideDoors == 1) {
                        IBusCommandGMDoorUnlockAll(context->ibus);
                    } else {
                        IBusCommandGMDoorUnlockHigh(context->ibus);
                    }
                }
            }
            context->bodyModuleStatus.lowSideDoors = 0;
        // If the engine was on, but now it's in position 1
        } else if (context->ibus->ignitionStatus >= IBUS_IGNITION_KL15 &&
                   ignitionStatus == IBUS_IGNITION_KLR
        ) {
            // Unlock the vehicle
            if (ConfigGetComfortUnlock() == CONFIG_SETTING_COMFORT_UNLOCK_POS_1 &&
                context->bodyModuleStatus.doorsLocked == 1
            ) {
                if (context->ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
                    IBusCommandGMDoorCenterLockButton(context->ibus);
                } else if (context->ibus->vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
                    if (context->bodyModuleStatus.lowSideDoors == 1) {
                        IBusCommandGMDoorUnlockAll(context->ibus);
                    } else {
                        IBusCommandGMDoorUnlockHigh(context->ibus);
                    }
                }
            }
        // If the ignition WAS off, but now it's not, then run these actions.
        // I realize the second condition is frivolous, but it helps with
        // readability.
        } else if (context->ibus->ignitionStatus == IBUS_IGNITION_OFF &&
                   ignitionStatus != IBUS_IGNITION_OFF
        ) {
            LogDebug(LOG_SOURCE_SYSTEM, "Handler: Ignition On");
            // Play a tone to wake up the WM8804 / PCM5122
            BC127CommandTone(Context.bt, "V 0 N C6 L 4");
            // Enable Telephone On
            TEL_ON = 1;
            // Anounce the CDC to the network
            HandlerIBusBroadcastCDCStatus(context);
            // Reset the metadata so we don't display the wrong data
            BC127ClearMetadata(context->bt);
            // Set the BT module connectable
            BC127CommandBtState(context->bt, BC127_STATE_ON, BC127_STATE_OFF);
            // Request BC127 state
            BC127CommandStatus(context->bt);
            BC127CommandList(context->bt);
            // Enable the TEL LEDs
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
                if (context->bt->activeDevice.avrcpLinkId == 0 &&
                    context->bt->activeDevice.a2dpLinkId == 0
                ) {
                    IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_RED);
                } else {
                    IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_GREEN);
                }
            }
            IBusCommandSetModuleStatus(
                context->ibus,
                IBUS_DEVICE_CDC,
                IBUS_DEVICE_LOC,
                0x01
            );
            context->cdChangerLastPoll = TimerGetMillis();
            // Ask the LCM for the redundant data
            LogDebug(LOG_SOURCE_SYSTEM, "Handler: Request LCM Redundant Data");
            IBusCommandLMGetRedundantData(context->ibus);
        }
    } else {
        if (ignitionStatus > IBUS_IGNITION_OFF) {
            HandlerIBusBroadcastCDCStatus(context);
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
                HandlerIBusBroadcastTELStatus(context, HANDLER_TEL_STATUS_FORCE);
                if (context->bt->activeDevice.avrcpLinkId != 0 &&
                    context->bt->activeDevice.a2dpLinkId != 0
                ) {
                    IBusCommandTELSetLED(
                        context->ibus,
                        IBUS_TEL_LED_STATUS_GREEN
                    );
                } else {
                    IBusCommandTELSetLED(
                        context->ibus,
                        IBUS_TEL_LED_STATUS_RED
                    );
                }
            }
            // Set the IKE to "found" if we haven't already to prevent
            // sending the telephone status multiple times
            context->ibusModuleStatus.IKE = 1;
        }
    }
    if (context->ibusModuleStatus.IKE == 0) {
        HandlerIBusBroadcastTELStatus(context, HANDLER_TEL_STATUS_FORCE);
        context->ibusModuleStatus.IKE = 1;
    }
}

/**
 * HandlerIBusIKESpeedRPMUpdate()
 *     Description:
 *         Act upon updates from the IKE about the vehicle speed / RPM
 *         * Lock the vehicle at 20mph
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusIKESpeedRPMUpdate(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char comfortLock = ConfigGetComfortLock();
    if (comfortLock != CONFIG_SETTING_OFF &&
        context->bodyModuleStatus.doorsLocked == 0x00
    ) {
        uint16_t speed = pkt[4] * 2;
        if ((comfortLock == CONFIG_SETTING_COMFORT_LOCK_10KM && speed >= 10) ||
            (comfortLock == CONFIG_SETTING_COMFORT_LOCK_20KM && speed >= 20)
        ) {
            if (context->ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
                IBusCommandGMDoorCenterLockButton(context->ibus);
            } else {
                IBusCommandGMDoorLockAll(context->ibus);
            }
        }
    }
}

/**
 * HandlerIBusIKEVehicleType()
 *     Description:
 *         Set the vehicle type
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The IBus Packet
 *     Returns:
 *         void
 */
void HandlerIBusIKEVehicleType(void *ctx, unsigned char *pkt)
{
    unsigned char rawVehicleType = (pkt[4] >> 4) & 0xF;
    unsigned char detectedVehicleType = IBusGetVehicleType(pkt);
    if (detectedVehicleType == 0xFF) {
        LogError("Handler: Unknown Vehicle Detected");
    }
    if (detectedVehicleType != ConfigGetVehicleType() &&
        detectedVehicleType != 0xFF
    ) {
        ConfigSetVehicleType(detectedVehicleType);
        if (rawVehicleType == 0x0A || rawVehicleType == 0x0F) {
            ConfigSetIKEType(IBUS_IKE_TYPE_LOW);
            LogDebug(LOG_SOURCE_SYSTEM, "Detected New Vehicle Type: E46/Z4");
        } else if (rawVehicleType == 0x02) {
            ConfigSetIKEType(IBUS_IKE_TYPE_LOW);
            LogDebug(
                LOG_SOURCE_SYSTEM,
                "Detected New Vehicle Type: E38/E39/E53 - Low OBC"
            );
        } else if (rawVehicleType == 0x00) {
            ConfigSetIKEType(IBUS_IKE_TYPE_HIGH);
            LogDebug(
                LOG_SOURCE_SYSTEM,
                "Detected New Vehicle Type: E38/E39/E53 - High OBC"
            );
        }
    }
}

/**
 * HandlerIBusLMIdentResponse()
 *     Description:
 *         Identify the light module variant
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *type - The light module variant
 *     Returns:
 *         void
 */
void HandlerIBusLMIdentResponse(void *ctx, unsigned char *variant)
{
    unsigned char lmVariant = *variant;
    if (ConfigGetLMVariant() != lmVariant) {
        ConfigSetLMVariant(lmVariant);
    }
}

/**
 * HandlerIBusLMLightStatus()
 *     Description:
 *         Track the Light Status messages in case the user has configured
 *         Three/Five One-Touch Blinkers.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusLMLightStatus(void *ctx, unsigned char *pkt)
{
    // Changed identifier as to not confuse it with the blink counter.
    uint8_t configBlinkLimit = ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS);

    if (configBlinkLimit > 1) {
        HandlerContext_t *context = (HandlerContext_t *) ctx;
        unsigned char lightStatus = pkt[4];
        unsigned char lightStatus2 = pkt[6];

        // Left blinker
        if (CHECK_BIT(lightStatus, IBUS_LM_LEFT_SIG_BIT) != 0 &&
            CHECK_BIT(lightStatus, IBUS_LM_RIGHT_SIG_BIT) == 0 &&
            CHECK_BIT(lightStatus2, IBUS_LM_BLINK_SIG_BIT) != 0
        ) {
            // If quickly switching blinker direction the LM will activate the
            // opposing blinker immediately, bypassing the "off" message.
            if (context->lightControlStatus.blinkStatus == HANDLER_LM_BLINK_RIGHT) {
                LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "LEFT > Quick Switch > Reset");
                context->lightControlStatus.blinkCount = 0;
            }
            context->lightControlStatus.blinkCount++;
            context->lightControlStatus.blinkStatus = HANDLER_LM_BLINK_LEFT;

            switch (context->lightControlStatus.comfortStatus) {
                case HANDLER_LM_COMF_BLINK_INACTIVE:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "LEFT > COMFORT_INACTIVE > %hhu/%hhu",
                        context->lightControlStatus.blinkCount,
                        configBlinkLimit
                    );
                    break;
                case HANDLER_LM_COMF_BLINK_LEFT:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "LEFT > COMFORT_LEFT > %hhu/%hhu",
                        context->lightControlStatus.blinkCount,
                        configBlinkLimit
                    );
                    if (context->lightControlStatus.blinkCount >= configBlinkLimit) {
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "LEFT > COMFORT_LEFT > Blink limit"
                        );
                        context->lightControlStatus.comfortStatus = HANDLER_LM_COMF_BLINK_INACTIVE;
                        IBusCommandLMActivateBulbs(context->ibus, IBUS_LM_BLINKER_OFF);
                    }
                    break;
                case HANDLER_LM_COMF_BLINK_RIGHT:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "LEFT > COMFORT_RIGHT > Cancel"
                    );
                    // If comfort blinkers are active, the first opposing blink
                    // will cancel comfort blinkers, and not count towards the blink count.
                    context->lightControlStatus.blinkCount = 0;
                    context->lightControlStatus.comfortStatus = HANDLER_LM_COMF_BLINK_INACTIVE;
                    IBusCommandLMActivateBulbs(context->ibus, IBUS_LM_BLINKER_OFF);
                    break;
                default:
                    LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "LEFT > Unknown State");
                    break;
            }
        } else if (CHECK_BIT(lightStatus, IBUS_LM_LEFT_SIG_BIT) != 0 &&
                   CHECK_BIT(lightStatus, IBUS_LM_RIGHT_SIG_BIT) == 0 &&
                   CHECK_BIT(lightStatus2, IBUS_LM_BLINK_SIG_BIT) == 0
        ) {
            LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "LEFT > Unrelated activity");
        } else if (CHECK_BIT(lightStatus, IBUS_LM_LEFT_SIG_BIT) == 0 &&
                  CHECK_BIT(lightStatus, IBUS_LM_RIGHT_SIG_BIT) != 0 &&
                  CHECK_BIT(lightStatus2, IBUS_LM_BLINK_SIG_BIT) != 0
        ) {
            if (context->lightControlStatus.blinkStatus == HANDLER_LM_BLINK_LEFT) {
                LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "RIGHT > Quick Switch > Reset");
                context->lightControlStatus.blinkCount = 0;
            }
            context->lightControlStatus.blinkCount++;
            context->lightControlStatus.blinkStatus = HANDLER_LM_BLINK_RIGHT;
            switch (context->lightControlStatus.comfortStatus) {
                case HANDLER_LM_COMF_BLINK_INACTIVE:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "RIGHT > COMFORT_INACTIVE > %hhu/%hhu",
                        context->lightControlStatus.blinkCount,
                        configBlinkLimit
                    );
                    break;
                case HANDLER_LM_COMF_BLINK_RIGHT:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "RIGHT > COMFORT_RIGHT > %hhu/%hhu",
                        context->lightControlStatus.blinkCount,
                        configBlinkLimit
                    );
                    if (context->lightControlStatus.blinkCount >= configBlinkLimit) {
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "RIGHT > COMFORT_RIGHT > Blink limit"
                        );
                        context->lightControlStatus.comfortStatus = HANDLER_LM_COMF_BLINK_INACTIVE;
                        IBusCommandLMActivateBulbs(context->ibus, IBUS_LM_BLINKER_OFF);
                    }
                    break;
                case HANDLER_LM_COMF_BLINK_LEFT:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "RIGHT > COMFORT_LEFT > Cancel"
                    );
                    context->lightControlStatus.blinkCount = 0;
                    context->lightControlStatus.comfortStatus = HANDLER_LM_COMF_BLINK_INACTIVE;
                    IBusCommandLMActivateBulbs(context->ibus, IBUS_LM_BLINKER_OFF);
                    break;
                default:
                    LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "RIGHT > Unknown State");
                    break;
            }
        } else if (CHECK_BIT(lightStatus, IBUS_LM_LEFT_SIG_BIT) == 0 &&
                   CHECK_BIT(lightStatus, IBUS_LM_RIGHT_SIG_BIT) != 0 &&
                   CHECK_BIT(lightStatus2, IBUS_LM_BLINK_SIG_BIT) == 0
        ) {
            LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "RIGHT > Unrelated activity");
        } else if (CHECK_BIT(lightStatus, IBUS_LM_LEFT_SIG_BIT) == 0 &&
                   CHECK_BIT(lightStatus, IBUS_LM_RIGHT_SIG_BIT) == 0
        ) {
            // OFF blinker (or anything non-blinker)
            // Only activate comfort blinkers after a single blink.
            if (context->lightControlStatus.blinkCount == 1) {
                LogDebug(
                    CONFIG_DEVICE_LOG_SYSTEM,
                    "OFF > Blinks: %hhu => Activate",
                    context->lightControlStatus.blinkCount
                );
                // I believe this is redundant, but better safe than sorry.
                switch (context->lightControlStatus.comfortStatus) {
                    case HANDLER_LM_COMF_BLINK_RIGHT:
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "OFF > Blinks: %hhu > COMFORT_RIGHT > Cancel",
                            context->lightControlStatus.blinkCount
                        );
                        context->lightControlStatus.comfortStatus = HANDLER_LM_COMF_BLINK_INACTIVE;
                        IBusCommandLMActivateBulbs(context->ibus, IBUS_LM_BLINKER_OFF);
                        break;
                    case HANDLER_LM_COMF_BLINK_LEFT:
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "OFF > Blinks: %hhu > COMFORT_LEFT > Cancel",
                            context->lightControlStatus.blinkCount
                        );
                        context->lightControlStatus.comfortStatus = HANDLER_LM_COMF_BLINK_INACTIVE;
                        IBusCommandLMActivateBulbs(context->ibus, IBUS_LM_BLINKER_OFF);
                        break;
                }
                // Activate comfort
                switch (context->lightControlStatus.blinkStatus) {
                    case HANDLER_LM_BLINK_LEFT:
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "OFF > Blinks: %hhu > BLINK_LEFT => Activate",
                            context->lightControlStatus.blinkCount
                        );
                        context->lightControlStatus.comfortStatus = HANDLER_LM_COMF_BLINK_LEFT;
                        IBusCommandLMActivateBulbs(context->ibus, IBUS_LM_BLINKER_LEFT);
                        break;
                    case HANDLER_LM_BLINK_RIGHT:
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "OFF > Blinks: %hhu > BLINK_RIGHT => Activate",
                            context->lightControlStatus.blinkCount
                        );
                        context->lightControlStatus.comfortStatus = HANDLER_LM_COMF_BLINK_RIGHT;
                        IBusCommandLMActivateBulbs(context->ibus, IBUS_LM_BLINKER_RIGHT);
                        break;
                }
            } else if (context->lightControlStatus.blinkCount > 1) {
                LogDebug(
                    CONFIG_DEVICE_LOG_SYSTEM,
                    "OFF > Blinks: %hhu => Do not activate comfort",
                    context->lightControlStatus.blinkCount
                );
                context->lightControlStatus.blinkCount = 0;
            } else {
                // Sequential non-blinker lamp activity
                LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "OFF > Unrelated activity!");
            }
            context->lightControlStatus.blinkStatus = HANDLER_LM_BLINK_OFF;
        }
    }
}

/**
 * HandlerIBusLCMDimmerStatus()
 *     Description:
 *         Track the Dimmer Status messages so we can correctly set the
 *         dimming state when messing with the lighting
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusLMDimmerStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint8_t checksum = IBusGetLMDimmerChecksum(pkt);
    if (checksum != context->lmDimmerChecksum) {
        IBusCommandDIAGetIOStatus(context->ibus, IBUS_DEVICE_LCM);
        context->lmLastIOStatus = TimerGetMillis();
        context->lmDimmerChecksum = checksum;
    }
}

/**
 * HandlerIBusLMRedundantData()
 *     Description:
 *         Check the VIN to see if we're in a new vehicle
 *         Raw: D0 10 80 54 50 4E 66 05 80 06 10 42 38 07 00 06 05 81
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusLMRedundantData(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char currentVehicleId[5] = {};
    ConfigGetVehicleIdentity(currentVehicleId);
    unsigned char vehicleId[] = {
        pkt[4],
        pkt[5],
        pkt[6],
        pkt[7],
        (pkt[8] >> 4) & 0xF,
    };
    // Check VIN
    if (memcmp(&vehicleId, &currentVehicleId, 5) != 0) {
        char vinTwo[] = {vehicleId[0], vehicleId[1], '\0'};
        LogWarning(
            "Detected VIN Change: %s%02x%02x%x",
            vinTwo,
            vehicleId[2],
            vehicleId[3],
            vehicleId[4]
        );
        // Request light module ident
        IBusCommandDIAGetIdentity(context->ibus, IBUS_DEVICE_LCM);
        // Save the new VIN
        ConfigSetVehicleIdentity(vehicleId);
        // Request the vehicle type
        IBusCommandIKEGetVehicleType(context->ibus);
        // Reset the Nav Type
        ConfigSetNavType(0x00);
        // Fallback to the CD53 UI as appropriate
        if (context->ibusModuleStatus.MID == 0 &&
            context->ibusModuleStatus.GT == 0 &&
            context->ibusModuleStatus.BMBT == 0
        ) {
            LogInfo(LOG_SOURCE_SYSTEM, "Fallback to CD53");
            HandlerSwitchUI(context, IBus_UI_CD53);
        }
    } else if (ConfigGetLMVariant() == CONFIG_SETTING_OFF) {
        // Identify the LM if we do not have an ID for it
        IBusCommandDIAGetIdentity(context->ibus, IBUS_DEVICE_LCM);
    }
}

/**
 * HandlerIBusMFLButton()
 *     Description:
 *         Act upon MFL button presses when in CD Changer mode (when BT is active)
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The packet
 *     Returns:
 *         void
 */
void HandlerIBusMFLButton(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char mflButton = pkt[IBUS_PKT_DB1];
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
        if (mflButton == IBUS_MFL_BTN_EVENT_VOICE_PRESS) {
            LogDebug(LOG_SOURCE_SYSTEM, "MFL OFF");
            context->mflButtonStatus = HANDLER_MFL_STATUS_OFF;
        }
        if (mflButton == IBUS_MFL_BTN_EVENT_VOICE_REL &&
            context->mflButtonStatus == HANDLER_MFL_STATUS_OFF
        ) {
            LogDebug(LOG_SOURCE_SYSTEM, "MFL PRESS");
            if (context->bt->callStatus == BC127_CALL_ACTIVE) {
                BC127CommandCallEnd(context->bt);
            } else if (context->bt->callStatus == BC127_CALL_INCOMING) {
                BC127CommandCallAnswer(context->bt);
            } else if (context->bt->callStatus == BC127_CALL_OUTGOING) {
                BC127CommandCallEnd(context->bt);
            } else if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING) {
                if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                    BC127CommandPause(context->bt);
                } else {
                    BC127CommandPlay(context->bt);
                }
            }
        } else if (mflButton == IBUS_MFL_BTN_EVENT_VOICE_HOLD) {
            LogDebug(LOG_SOURCE_SYSTEM, "MFL HOLD");
            context->mflButtonStatus = HANDLER_MFL_STATUS_SPEAK_HOLD;
            LogDebug(LOG_SOURCE_SYSTEM, "Toggle VR");
            BC127CommandToggleVR(context->bt);
        }
    } else {
        if (mflButton == IBUS_MFL_BTN_EVENT_VOICE_REL) {
            if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING) {
                if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                    BC127CommandPause(context->bt);
                } else {
                    BC127CommandPlay(context->bt);
                }
            }
        }
    }
}

/**
 * HandlerIBusModuleStatusRequest()
 *     Description:
 *         Respond to module status requests for those modules which
 *         we are emulating
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void HandlerIBusModuleStatusRequest(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_CDC) {
        IBusCommandSetModuleStatus(
            context->ibus,
            IBUS_DEVICE_CDC,
            pkt[IBUS_PKT_SRC],
            0x00
        );
        context->cdChangerLastPoll = TimerGetMillis();
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_TEL &&
               ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON
    ) {
        IBusCommandSetModuleStatus(
            context->ibus,
            IBUS_DEVICE_TEL,
            pkt[IBUS_PKT_SRC],
            IBUS_TEL_SIG_EVEREST
        );
    }
}


/**
 * HandlerIBusRADVolumeChange()
 *     Description:
 *         Adjust the volume for calls based on where the user is adjusting
 *         the audio volume.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void HandlerIBusRADVolumeChange(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint8_t direction = pkt[IBUS_PKT_DB1] & 0xF;
    // Only watch for changes when not on a call
    if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_HANDSFREE) {
        uint8_t steps = pkt[IBUS_PKT_DB1] >> 4;
        unsigned char volume = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
        if (direction == IBUS_RAD_VOLUME_DOWN) {
            while (steps > 0 && volume < HANDLER_TEL_VOL_OFFSET_MAX) {
                volume = volume + 1;
                steps--;
            }
        } else if (direction == IBUS_RAD_VOLUME_UP) {
            while (steps > 0 && volume > 0) {
                volume = volume - 1;
                steps--;
            }
        }
        ConfigSetSetting(CONFIG_SETTING_TEL_VOL, volume);
    }
}

/**
 * HandlerIBusTELVolumeChange()
 *     Description:
 *         Adjust the volume for calls when asked to do so by the BMBT / MID.
 *         If the vehicle has a DSP or MID, then forward those changes to the DSP
 *         or RAD rather than adjusting the DAC volume.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void HandlerIBusTELVolumeChange(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint8_t direction = pkt[IBUS_PKT_DB1] & 0x0F;
    // Forward volume changes to the RAD / DSP when in Bluetooth mode OR
    // when in radio mode if the S/PDIF input is selected and the DSP is found
    if ((context->uiMode == IBus_UI_BMBT ||
         context->uiMode == IBus_UI_MID ||
         context->uiMode == IBus_UI_MID_BMBT)
        && (
            context->ibus->cdChangerFunction != IBUS_CDC_FUNC_NOT_PLAYING ||
            (
                ConfigGetSetting(CONFIG_SETTING_USE_SPDIF_INPUT) == CONFIG_SETTING_ON &&
                context->ibusModuleStatus.DSP == 1
            )
        )
    )  {
        unsigned char sourceSystem = IBUS_DEVICE_BMBT;
        if (context->ibusModuleStatus.MID == 1) {
            sourceSystem = IBUS_DEVICE_MID;
        }
        IBusCommandSetVolume(
            context->ibus,
            sourceSystem,
            IBUS_DEVICE_RAD,
            pkt[4]
        );
        uint8_t steps = pkt[IBUS_PKT_DB1] >> 4;
        unsigned char volume = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
        if (direction == IBUS_RAD_VOLUME_UP) {
            while (steps > 0 && volume < HANDLER_TEL_VOL_OFFSET_MAX) {
                volume = volume + 1;
                steps--;
            }
        } else if (direction == IBUS_RAD_VOLUME_DOWN) {
            while (steps > 0 && volume > 0) {
                volume = volume - 1;
                steps--;
            }
        }
        ConfigSetSetting(CONFIG_SETTING_TEL_VOL, volume);
    } else if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING) {
        unsigned char volumeConfig = CONFIG_SETTING_DAC_TEL_TCU_MODE_VOL;
        unsigned char volume = ConfigGetSetting(volumeConfig);
        // PCM51XX volume gets lower as you raise the value in the register
        if (direction == IBUS_RAD_VOLUME_UP && volume > 0x00) {
            volume = volume - 1;
        } else if (IBUS_RAD_VOLUME_DOWN == 0 && volume < 0xFF) {
            volume = volume + 1;
        }
        PCM51XXSetVolume(volume);
        ConfigSetSetting(volumeConfig, volume);
    }
}

/**
 * HandlerIBusModuleStatusResponse()
 *     Description:
 *         Track module status as we get them & track UI changes
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The packet
 *     Returns:
 *         void
 */
void HandlerIBusModuleStatusResponse(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char module = pkt[IBUS_PKT_SRC];
    if (module == IBUS_DEVICE_DSP &&
        context->ibusModuleStatus.DSP == 0
    ) {
        context->ibusModuleStatus.DSP = 1;
        LogInfo(LOG_SOURCE_SYSTEM, "DSP Detected");
    } else if (module == IBUS_DEVICE_GT &&
        context->ibusModuleStatus.GT == 0
    ) {
        context->ibusModuleStatus.GT = 1;
        LogInfo(LOG_SOURCE_SYSTEM, "GT Detected");
        unsigned char uiMode = ConfigGetUIMode();
        if (uiMode != IBus_UI_BMBT &&
            uiMode != IBus_UI_MID_BMBT &&
            uiMode != IBus_UI_BUSINESS_NAV
        ) {
            // Request the Navigation Identity
            IBusCommandDIAGetIdentity(context->ibus, IBUS_DEVICE_GT);
            IBusCommandDIAGetOSIdentity(context->ibus, IBUS_DEVICE_GT);
        }
    } else if (module == IBUS_DEVICE_LCM &&
        context->ibusModuleStatus.LCM == 0
    ) {
        LogInfo(LOG_SOURCE_SYSTEM, "LCM Detected");
        context->ibusModuleStatus.LCM = 1;
    } else if (module == IBUS_DEVICE_MID &&
        context->ibusModuleStatus.MID == 0
    ) {
        context->ibusModuleStatus.MID = 1;
        LogInfo(LOG_SOURCE_SYSTEM, "MID Detected");
        unsigned char uiMode = ConfigGetUIMode();
        if (uiMode != IBus_UI_MID &&
            uiMode != IBus_UI_MID_BMBT
        ) {
            if (context->ibusModuleStatus.GT == 1) {
                LogInfo(LOG_SOURCE_SYSTEM, "Detected MID / BMBT UI");
                HandlerSwitchUI(context, IBus_UI_MID_BMBT);
            } else {
                LogInfo(LOG_SOURCE_SYSTEM, "Detected MID UI");
                HandlerSwitchUI(context, IBus_UI_MID);
            }
        }
    } else if (module == IBUS_DEVICE_BMBT &&
        context->ibusModuleStatus.BMBT == 0
    ) {
        context->ibusModuleStatus.BMBT = 1;
        LogInfo(LOG_SOURCE_SYSTEM, "BMBT Detected");
        // If a BMBT is in the car, we probably have a Nav...
        unsigned char uiMode = ConfigGetUIMode();
        if (uiMode != IBus_UI_BMBT &&
            uiMode != IBus_UI_MID_BMBT &&
            uiMode != IBus_UI_BUSINESS_NAV
        ) {
            // Request the Navigation Identity
            IBusCommandDIAGetIdentity(context->ibus, IBUS_DEVICE_GT);
            IBusCommandDIAGetOSIdentity(context->ibus, IBUS_DEVICE_GT);
        }
    } else if (module == IBUS_DEVICE_RAD &&
        context->ibusModuleStatus.RAD == 0
    ) {
        context->ibusModuleStatus.RAD = 1;
        LogInfo(LOG_SOURCE_SYSTEM, "RAD Detected");
    }
}

/**
 * HandlerIBusBroadcastCDCStatus()
 *     Description:
 *         Wrapper to send the CDC Status
 *     Params:
 *         HandlerContext_t *context - The handler context
 *     Returns:
 *         void
 */
void HandlerIBusBroadcastCDCStatus(HandlerContext_t *context)
{
    unsigned char curStatus = IBUS_CDC_STAT_STOP;
    if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PAUSE) {
        curStatus = IBUS_CDC_FUNC_PAUSE;
    } else if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING) {
        curStatus = IBUS_CDC_STAT_PLAYING;
    }
    unsigned char discCount = IBUS_CDC_DISC_COUNT_6;
    // Report disc 7 loaded so any button press causes a CD Changer command
    // to be sent by the RAD (since there is no 7th disc)
    unsigned char discNumber = 0x07;
    if (context->uiMode == IBus_UI_BMBT) {
        discCount = IBUS_CDC_DISC_COUNT_1;
        discNumber = 0x01;
    }
    IBusCommandCDCStatus(
        context->ibus,
        curStatus,
        context->ibus->cdChangerFunction,
        discCount,
        discNumber
    );
    context->cdChangerLastStatus = TimerGetMillis();
}

/**
 * HandlerIBusBroadcastTELStatus()
 *     Description:
 *         Send the TEL status to the vehicle
 *     Params:
 *         HandlerContext_t *context - The module context
 *         unsigned char sendFlag - Weather to update the status only if it is
 *             different, or force broadcasting it.
 *     Returns:
 *         uint8_t - Returns 1 if the status has changed, zero otherwise
 */
uint8_t HandlerIBusBroadcastTELStatus(
    HandlerContext_t *context,
    unsigned char sendFlag
) {
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
        unsigned char currentTelStatus = 0x00;
        if (context->bt->scoStatus == BC127_CALL_SCO_OPEN) {
            currentTelStatus = IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE;
        } else {
            currentTelStatus = IBUS_TEL_STATUS_ACTIVE_POWER_HANDSFREE;
        }
        if (context->telStatus != currentTelStatus ||
            sendFlag == HANDLER_TEL_STATUS_FORCE
        ) {
            context->telStatus = currentTelStatus;
            // Do not set the active call flag for these UIs to allow
            // the radio volume controls to remain active
            if (currentTelStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE &&
                (context->uiMode == IBus_UI_CD53 ||
                 context->uiMode == IBus_UI_BUSINESS_NAV)
            ) {
                return 1;
            }
            IBusCommandTELStatus(context->ibus, currentTelStatus);
            return 1;
        }
    }
    return 0;
}

/**
 * HandlerTimerBC127State()
 *     Description:
 *         Ensure the BC127 has booted, and if not, blink the red TEL LED
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerBC127State(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->powerState == BC127_STATE_OFF &&
        context->btBootFailure == HANDLER_BT_BOOT_OK
    ) {
        LogWarning("BC127 Boot Failure");
        uint16_t bootFailCount = ConfigGetBC127BootFailures();
        bootFailCount++;
        ConfigSetBC127BootFailures(bootFailCount);
        IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_RED_BLINKING);
        context->btBootFailure = HANDLER_BT_BOOT_FAIL;
    }
}

/**
 * HandlerTimerCDCAnnounce()
 *     Description:
 *         This periodic task tracks how long it has been since the radio
 *         sent us (the CDC) a "ping". We should re-announce ourselves if that
 *         value reaches the timeout specified and the ignition is on.
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerCDCAnnounce(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint32_t now = TimerGetMillis();
    if ((now - context->cdChangerLastPoll) >= HANDLER_CDC_ANOUNCE_TIMEOUT &&
        context->ibus->ignitionStatus > IBUS_IGNITION_OFF
    ) {
        IBusCommandSetModuleStatus(
            context->ibus,
            IBUS_DEVICE_CDC,
            IBUS_DEVICE_LOC,
            0x01
        );
        context->cdChangerLastPoll = now;
    }
}

/**
 * HandlerTimerCDCSendStatus()
 *     Description:
 *         This periodic task will proactively send the CDC status to the BM5x
 *         radio if we don't see a status poll within the last 20000 milliseconds.
 *         The CDC poll happens every 19945 milliseconds
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerCDCSendStatus(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint32_t now = TimerGetMillis();
    if ((now - context->cdChangerLastStatus) >= HANDLER_CDC_STATUS_TIMEOUT &&
        context->ibus->ignitionStatus > IBUS_IGNITION_OFF &&
        (context->uiMode == IBus_UI_BMBT || context->uiMode == IBus_UI_MID_BMBT)
    ) {
        HandlerIBusBroadcastCDCStatus(context);
        LogDebug(LOG_SOURCE_SYSTEM, "Handler: Send CDC status preemptively");
    }
}

/**
 * HandlerTimerDeviceConnection()
 *     Description:
 *         Monitor the BT connection and ensure it stays connected
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerDeviceConnection(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (strlen(context->bt->activeDevice.macId) > 0 &&
        context->bt->activeDevice.a2dpLinkId == 0
    ) {
        if (context->btDeviceConnRetries <= HANDLER_DEVICE_MAX_RECONN) {
            LogDebug(
                LOG_SOURCE_SYSTEM,
                "Handler: A2DP link closed -- Attempting to connect"
            );
            BC127CommandProfileOpen(
                context->bt,
                context->bt->activeDevice.macId,
                "A2DP"
            );
            context->btDeviceConnRetries += 1;
        } else {
            LogError("Handler: Giving up on BT connection");
            context->btDeviceConnRetries = 0;
            // Enable connectivity
            BC127CommandBtState(context->bt, BC127_STATE_ON, context->bt->discoverable);
            BC127ClearPairedDevices(context->bt);
            BC127CommandClose(context->bt, BC127_CLOSE_ALL);
        }
    } else if (context->btDeviceConnRetries > 0) {
        context->btDeviceConnRetries = 0;
    }
}

/**
 * HandlerTimerLCMIOStatus()
 *     Description:
 *         Request the LCM I/O Status when the key is in position 2 or above
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerLCMIOStatus(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint32_t now = TimerGetMillis();
    if (context->ibusModuleStatus.LCM != 0 &&
        context->ibus->ignitionStatus != IBUS_IGNITION_OFF &&
        (now - context->lmLastIOStatus) >= 20000
    ) {
        // Ask the LCM for the I/O Status of all lamps
        IBusCommandDIAGetIOStatus(context->ibus, IBUS_DEVICE_LCM);
        context->lmLastIOStatus = now;
    }
}

/**
 * HandlerTimerOpenProfileErrors()
 *     Description:
 *         If there are any profile open errors, request the profile
 *         be opened again
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerOpenProfileErrors(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (strlen(context->bt->activeDevice.macId) > 0) {
        uint8_t idx;
        for (idx = 0; idx < BC127_PROFILE_COUNT; idx++) {
            if (context->bt->pairingErrors[idx] == 1) {
                LogDebug(LOG_SOURCE_SYSTEM, "Handler: Attempting to resolve pairing error");
                BC127CommandProfileOpen(
                    context->bt,
                    context->bt->activeDevice.macId,
                    PROFILES[idx]
                );
                context->bt->pairingErrors[idx] = 0;
            }
        }
    }
}

/**
 * HandlerTimerPoweroff()
 *     Description:
 *         Track the time since the last I-Bus message and see if we need to
 *         power off.
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerPoweroff(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (ConfigGetSetting(CONFIG_SETTING_AUTO_POWEROFF) == CONFIG_SETTING_ON) {
        uint32_t lastRx = TimerGetMillis() - context->ibus->rxLastStamp;
        if (lastRx >= HANDLER_POWER_TIMEOUT_MILLIS) {
            if (context->powerStatus == HANDLER_POWER_ON) {
                // Destroy the UART module for IBus
                UARTDestroy(IBUS_UART_MODULE);
                TimerDelayMicroseconds(500);
                LogInfo(LOG_SOURCE_SYSTEM, "System Power Down!");
                context->powerStatus = HANDLER_POWER_OFF;
                // Disable the TH3122
                IBUS_EN = 0;
            } else {
                // Re-enable the TH3122 EN line so we can try pulling it,
                // and the regulator low again
                IBUS_EN = 1;
                context->powerStatus = HANDLER_POWER_ON;
            }
        }
    }
}

/**
 * HandlerTimerScanDevices()
 *     Description:
 *         Rescan for devices on the PDL periodically. Scan every 5 seconds if
 *         there is no connected device, otherwise every 60 seconds
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerScanDevices(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (((context->bt->activeDevice.deviceId == 0 &&
        context->btConnectionStatus == HANDLER_BT_CONN_OFF) ||
        context->scanIntervals == 12) &&
        context->ibus->ignitionStatus > IBUS_IGNITION_OFF
    ) {
        context->scanIntervals = 0;
        BC127ClearInactivePairedDevices(context->bt);
        BC127CommandList(context->bt);
    } else {
        context->scanIntervals += 1;
    }
}
