#! /usr/bin/python
# -*- encoding: iso-8859-15 -*-
# vim: syntax=python sw=4 sts=4 et
#
#   This work is held in copyright as an unpublished work by
#   Silicon Graphics, Inc.  All rights reserved.
#
#   Copyright (c) 2012-2012 Silicon Graphics, Inc.
#   All rights reserved.
#
#   Michel Bourget <michel at sgi.com>
#

EVENT2_UNSPECIFIED  = 0
EVENT2_TRIGGER      = 1
EVENT2_OEM          = 2
EVENT2_SENSOR       = 3
EVENT2_PREVIOUS     = 4
EVENT2_RESERVED     = 9

EVENT3_UNSPECIFIED  = 0
EVENT3_THRESHOLD    = 1
EVENT3_OEM          = 2
EVENT3_SENSOR       = 3
EVENT3_RESERVED     = 9

SDR_RECORD_TYPE_FULL_SENSOR           = 0x01
SDR_RECORD_TYPE_COMPACT_SENSOR        = 0x02
SDR_RECORD_TYPE_EVENTONLY_SENSOR      = 0x03
SDR_RECORD_TYPE_ENTITY_ASSOC          = 0x08
SDR_RECORD_TYPE_DEVICE_ENTITY_ASSOC   = 0x09
SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR  = 0x10
SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR    = 0x11
SDR_RECORD_TYPE_MC_DEVICE_LOCATOR     = 0x12
SDR_RECORD_TYPE_MC_CONFIRMATION       = 0x13
SDR_RECORD_TYPE_BMC_MSG_CHANNEL_INFO  = 0x14
SDR_RECORD_TYPE_OEM                   = 0xC0
SDR_FULL_SENSOR_LENGTH_OFFSET         = 0x2F # 47
SDR_COMPACT_SENSOR_OFFSET             = 0x1F # 31
SDR_EVENTONLY_SENSOR_OFFSET           = 0x10 # 16

REC_LEN_OFFSET = 5
LENGTH_MASK    = 0x0F



table_29_6 = {
    'threshold': {
        'event1': {
              'byte2': {
                  'mask':   0xC0
                , 'shift':  6
                ,  0:  EVENT2_UNSPECIFIED
                ,  1:  EVENT2_TRIGGER
                ,  2:  EVENT2_OEM
                ,  3:  EVENT2_SENSOR
                }
            , 'byte3': {
                  'mask':   0x30
                , 'shift':  4
                ,  0:  EVENT3_UNSPECIFIED
                ,  1:  EVENT3_THRESHOLD
                ,  2:  EVENT3_OEM
                ,  3:  EVENT3_SENSOR
                }
            , 'offset': {
                  'mask':   0x0F
                , 'shift':  0
                , 'data': True
            }
        , 'event2': {
              'mask': 0xFF
            , 'data':True
            }
        , 'event3': {
              'mask': 0xFF
            , 'data': True
            }
        }
      }
    , 'discrete': {
        'event1': {
              'byte2': {
                  'mask':   0xC0
                , 'shift':  6
                ,  0:  EVENT2_UNSPECIFIED
                ,  1:  EVENT2_PREVIOUS
                ,  2:  EVENT2_OEM
                ,  3:  EVENT2_SENSOR
                }
            , 'byte3': {
                  'mask':   0x30
                , 'shift':  6
                ,  0:  EVENT3_UNSPECIFIED
                ,  1:  EVENT3_RESERVED
                ,  2:  EVENT3_OEM
                ,  3:  EVENT3_SENSOR
                }
            , 'offset': {
                  'mask':   0x0F
                , 'shift':  6
                , 'data':   True
            }
        }
      }
    , 'OEM': {  # XXX: Should not be used since 42_1 OEM stuff is disabled
        'event1': {
              'byte2': {
                  'mask':   0xC0
                , 'shift':  6
                ,  0:  EVENT2_UNSPECIFIED
                ,  1:  EVENT2_PREVIOUS
                ,  2:  EVENT2_OEM
                ,  3:  EVENT2_RESERVED
                }
            , 'byte3': {
                  'mask':   0x30
                , 'shift':  6
                ,  0:  EVENT3_UNSPECIFIED
                ,  1:  EVENT3_RESERVED
                ,  2:  EVENT3_OEM
                ,  3:  EVENT3_RESERVED
                }
            , 'offset': {
                  'mask':   0x0F
                , 'shift':  6
                , 'data':   True
            }
        }
    }
}

table_42_1 = {
      'unspecified': {
              'enabled':        0
            , 'from':           0x00
            , 'to':             0x00
            , 'name':           'unspecified'   # no-op
            , 'table':          None
            }
    , 'threshold': {
              'enabled':        1
            , 'from':           0x01
            , 'to':             0x01
            , 'name':           'threshold'     # table_29_6['threshold']
            , 'table':          'table_42_2'
        }
    , 'generic': {
              'enabled':        1
            , 'from':           0x02
            , 'to':             0x0c
            , 'name':           'discrete'      # table_29_6['discrete']
            , 'table':          'table_42_2'
        }
    , 'sensor-specific': {
              'enabled':        1
            , 'from':           0x6f
            , 'to':             0x6f
            , 'name':           'discrete'      # table_29_6['discrete']
            , 'table':          'table_42_3'
        }
    , 'OEM': {
              'enabled':        0
            , 'from':           0x70
            , 'to':             0x7f
            , 'name':           'OEM'           # table_29_6['OEM']
            , 'table':          None
        }
    }

table_42_2 = {  # Based on eventType
      0x01: {
              'enabled':        1
            , 'name':           'threshold'
            , 0x00:             'lncGoingLow'
            , 0x01:             'lncGoingHigh'
            , 0x02:             'lcGoingLow'
            , 0x03:             'lcGoingHigh'
            , 0x04:             'lnrGoingLow'
            , 0x05:             'lnrGoingHigh'
            , 0x06:             'uncGoingLow'
            , 0x07:             'uncGoingHigh'
            , 0x08:             'ucGoingLow'
            , 0x09:             'ucGoingHigh'
            , 0x0a:             'unrGoingLow'
            , 0x0b:             'unrGoingHigh'
      }
    , 0x02: {
              'enabled':        1
            , 'name':           'discrete'
            , 0x00:             'transitionToIdle'
            , 0x01:             'transitionToActive'
            , 0x02:             'transitionToBusy'
            }
    , 0x03: {
              'enabled':        1
            , 'name':           'digitalDiscrete'
            , 0x00:             'stateDeasserted'
            , 0x01:             'stateAsserted'
            }
    , 0x04: {
              'enabled':        1
            , 'name':           'digitalDiscrete'
            , 0x00:             'predictiveFailureDeasserted'
            , 0x01:             'predictiveFailureAsserted'
            }
    , 0x05: {
              'enabled':        1
            , 'name':           'digitalDiscrete'
            , 0x00:             'limitNotExceeded'
            , 0x01:             'limitExceeded'
            }
    , 0x06: {
              'enabled':        1
            , 'name':           'digitalDiscrete'
            , 0x00:             'performanceMet'
            , 0x01:             'performanceLags'
            }
    , 0x07: {
              'enabled':        1
            , 'name':           'discrete'
            , 0x00:             'transitionToOK'
            , 0x01:             'transitionToNonCriticalFromOK'
            , 0x02:             'transitionToCriticalFromLessSevere'
            , 0x03:             'transitionToNonRecoverableFromLessSevere'
            , 0x04:             'transitionToNonCriticalFromMoreSevere'
            , 0x05:             'transitionToCriticalFromNonRecoverrable'
            , 0x06:             'transitionToNonRecoverable'
            , 0x07:             'monitor'
            , 0x08:             'informational'
            }
    , 0x08: {
              'enabled':        1
            , 'name':           'digitalDiscrete'
            , 0x00:             'deviceRemovedAbsent'
            , 0x01:             'deviceInsertedPresent'
            }
    , 0x09: {
              'enabled':        1
            , 'name':           'digitalDiscrete'
            , 0x00:             'deviceDisabled'
            , 0x01:             'deviceEnabled'
            }
    , 0x0a: {
              'enabled':        1
            , 'name':           'discrete'
            , 0x00:             'transitionToRunning'
            , 0x01:             'transitionToIntest'
            , 0x02:             'transitionToPowerOff'
            , 0x03:             'transitionToOnLine'
            , 0x04:             'transitionToOffLine'
            , 0x05:             'transitionToOffDuty'
            , 0x06:             'transitionToDegraded'
            , 0x07:             'transitionToPowerSave'
            , 0x08:             'internalError'
            }
    , 0x0b: {
              'enabled':        1
            , 'name':           'discrete'
            , 0x00:             'redundacyFull'
            , 0x01:             'redundacyLost'
            , 0x02:             'redundacyDegraded'
            , 0x03:             'redundacyNoSufficientResourcesFromRedundant'
            , 0x04:             'redundacyNoSufficientResourcesFromInsufficientResources'
            , 0x05:             'redundacyNoInsufficientResources'
            , 0x06:             'redundacyDegradedFromFully'
            , 0x07:             'redundacyDegradedFromNonRedundant'
            }
    , 0x0c: {
              'enabled':        1
            , 'name':           'discrete'
            , 0x00:             'acpiD0PowerState'
            , 0x01:             'acpiD1PowerState'
            , 0x02:             'acpiD2PowerState'
            , 0x03:             'acpiD3PowerState'
            }
    }

table_42_3 = {  # Based on sensorType
      0x00: {
              'enabled':        0
            , 'name':           'reserved'
            }
    , 0x01: {
              'enabled':        1
            , 'name':           'temperature'
            }
    , 0x02: {
              'enabled':        1
            , 'name':           'voltage'
            }
    , 0x03: {
              'enabled':        1
            , 'name':           'current'
            }
    , 0x04: {
              'enabled':        1
            , 'name':           'fan'
            }
    , 0x05: {
              'enabled':        1
            , 'name':           'chassisIntrusion'
            , 0x00:             'generalChassisIntrusion'
            , 0x01:             'driveBayIntrusion'
            , 0x02:             'ioCardAreaIntrusion'
            , 0x03:             'processorAreaIntrusion'
            , 0x04:             'lanLeashLost'
            , 0x05:             'unauthorizedDockUndock'
            , 0x06:             'fanAreaIntrusion'
            }
    , 0x06: {
              'enabled':        1
            , 'name':           'platformSecurityViolationAttempt'
            , 0x00:             'frontPanelViolationAttempt'
            , 0x01:             'userPasswordViolation'
            , 0x02:             'setupPasswordViolation'
            , 0x03:             'networkPasswordViolation'
            , 0x04:             'otherPasswordViolation'
            , 0x05:             'outOfBandPasswordViolation'
            }
    , 0x07: {
              'enabled':        1
            , 'name':           'processor'
            , 0x00:             'iErr'
            , 0x01:             'thermalTrip'
            , 0x02:             'bistFailure'
            , 0x03:             'hangPost'
            , 0x04:             'initFailure'
            , 0x05:             'configError'
            , 0x06:             'uncorrectableCpuComplexError'
            , 0x07:             'processorPresenceDetected'
            , 0x08:             'processorDisabled'
            , 0x09:             'terminatorPresenceDetected'
            , 0x0a:             'processorThrottled'
            , 0x0b:             'uncorrectableMachineCheckException'
            , 0x0c:             'correctableMachineCheckError'
            }
    , 0x08: {
              'enabled':        1
            , 'name':           'powerSupply'
            , 0x00:             'presenceDetected'
            , 0x01:             'powerSupplyFailureDetected'
            , 0x02:             'predictiveFailure'
            , 0x03:             'powerSupplyInputLostAcDC'
            , 0x04:             'powerSupplyInputLostOrOutOfRange'
            , 0x05:             'powerSupplyInputtOrOutOfRangeButPresent'
            , 0x06:             'configurationError'
            }
    , 0x09: {
              'enabled':        1
            , 'name':           'powerUnit'
            , 0x00:             'powerOffDown'
            , 0x01:             'powerCycle'
            , 0x02:             'powerDown240VA'
            , 0x03:             'powerDownInterlock'
            , 0x04:             'powerAcLost'
            , 0x05:             'softPowerControlFailure'
            , 0x06:             'powerUnitFailureDetected'
            , 0x07:             'powerUnitPredictiveFailure'
            }
    , 0x0a: {
              'enabled':        1
            , 'name':           'coolingDevice'
            }
    , 0x0b: {
              'enabled':        1
            , 'name':           'otherUnitsBaseSensor'
            }
    , 0x0c: {
              'enabled':        1
            , 'name':           'memory'
            , 0x00:             'correctableECC'
            , 0x01:             'uncorrectableECC'
            , 0x02:             'parity'
            , 0x03:             'memoryScrubFailed'
            , 0x04:             'memoryDeviceDisabled'
            , 0x05:             'correctableECCErrorLoggingLimitReached'
            , 0x06:             'presenceDetected'
            , 0x07:             'configurationError'
            , 0x08:             'spare'
            , 0x09:             'memoryThrottled'
            , 0x0a:             'criticalOverTemperature'
            }
    , 0x0d: {
              'enabled':        1
            , 'name':           'driveSlot'
            , 0x00:             'drivePresence'
            , 0x01:             'driveFault'
            , 0x02:             'drivePredictiveFailure'
            , 0x03:             'driveHotSpare'
            , 0x04:             'driveConsistencyParityCheckInProgress'
            , 0x05:             'driveInCriticalArray'
            , 0x06:             'driveInFailedArray'
            , 0x07:             'driveRebuildRemapInProgress'
            , 0x07:             'driveRebuildRemapAborted'
            }
    , 0x0e: {
              'enabled':        1
            , 'name':           'postMemoryResize'
            , 0x00:             'drivePresence'
            }
    , 0x0f: {
              'enabled':        1
            , 'name':           'postError'
            , 0x00:             'postError'
            , 0x01:             'postHang'
            , 0x02:             'postProgress'
            }
    , 0x10: {
              'enabled':        1
            , 'name':           'eventLoggingDisabled'
            , 0x00:             'correctableMemoryErrorLoggingDisabled'
            , 0x01:             'eventTypeLoggingDisabled'
            , 0x02:             'logAreaResetCleared'
            , 0x03:             'allEventLoggingDisabled'
            , 0x04:             'selFull'
            , 0x05:             'selAlmostFull'
            , 0x06:             'correctableMachineCheckErrorLoggingDisabled'
            }
    , 0x11: {
              'enabled':        1
            , 'name':           'watchdog1'
            , 0x00:             'biosWatchdogReset'
            , 0x01:             'osWatchdogReset'
            , 0x02:             'osWatchdogShutDown'
            , 0x03:             'osWatchdogPowerDown'
            , 0x04:             'osWatchdogPowerCycle'
            , 0x05:             'osWatchdogNMI'
            , 0x06:             'osWatchdogExpired'
            , 0x07:             'osWatchdogNonNMI'
            }
    , 0x12: {
              'enabled':        1
            , 'name':           'systemEvent'
            , 0x00:             'systemReconfigured'
            , 0x01:             'OEMSystemBootEvent'
            , 0x02:             'undeterminedSystemHardwareFailureEvent'
            , 0x03:             'auxiliaryLogEntryAdded'
            , 0x04:             'pefAction'
            , 0x05:             'timestampClockSynch'
            }
    , 0x13: {
              'enabled':        1
            , 'name':           'criticalInterrupt'
            , 0x00:             'NMIfrontPanel'
            , 0x01:             'busTimeout'
            , 0x02:             'ioChannelCheckNMI'
            , 0x03:             'softwareNMI'
            , 0x04:             'pciPerr'
            , 0x05:             'pciSerr'
            , 0x06:             'eisaFailSafeTimeout'
            , 0x07:             'busCorrectableError'
            , 0x08:             'busUncorrectableError'
            , 0x09:             'fatalNMI'
            , 0x0a:             'busFatalError'
            , 0x0b:             'busDegraded'
            }
    , 0x14: {
              'enabled':        1
            , 'name':           'buttonSwitch'
            , 0x00:             'powerButtonPressed'
            , 0x01:             'sleepButtonPressed'
            , 0x02:             'resetButtonPressed'
            , 0x03:             'fruLatchedOpen'
            , 0x04:             'fruServiceRequestButton'
            }
    , 0x15: {
              'enabled':        1
            , 'name':           'moduleBoard'
            }
    , 0x16: {
              'enabled':        1
            , 'name':           'microcontrollerCoprocessor'
            }
    , 0x17: {
              'enabled':        1
            , 'name':           'addInCard'
            }
    , 0x18: {
              'enabled':        1
            , 'name':           'chassis'
            }
    , 0x19: {
              'enabled':        1
            , 'name':           'chipSet'
            , 0x00:             'softPowerControlFailure'
            , 0x01:             'thermalTrip'
            }
    , 0x1a: {
              'enabled':        1
            , 'name':           'otherFru'
            }
    , 0x1b: {
              'enabled':        1
            , 'name':           'cableInterconnect'
            , 0x00:             'cableInterconnectConnected'
            , 0x01:             'configError'
            }
    , 0x1c: {
              'enabled':        1
            , 'name':           'terminator'
            }
    , 0x1d: {
              'enabled':        1
            , 'name':           'systemBoot'
            , 0x00:             'initiatedPowerUp'
            , 0x01:             'initiatedHardReset'
            , 0x02:             'initiatedWarmReset'
            , 0x03:             'userPxeRequest'
            , 0x04:             'automaticBootToDiagnostic'
            , 0x05:             'osRunTimeSoftwareInitiatedHardReset'
            , 0x06:             'osRunTimeSoftwareInitiatedWarmReset'
            , 0x07:             'systremRestart'
            }
    , 0x1e: {
              'enabled':        1
            , 'name':           'bootError'
            , 0x00:             'noBootableMedia'
            , 0x01:             'nonBootableDiskette'
            , 0x02:             'pxeServerNotFound'
            , 0x03:             'invalidBootSector'
            , 0x04:             'timeoutWaitingForBootSourceUserSelection'
            }
    , 0x1f: {
              'enabled':        1
            , 'name':           'osBoot'
            , 0x00:             'driveAbootCompleted'
            , 0x01:             'driveCbootCompleted'
            , 0x02:             'pxeBootCompleted'
            , 0x03:             'diagnosticBootCompleted'
            , 0x04:             'cdRomBootCompleted'
            , 0x05:             'romBootCompleted'
            , 0x06:             'bootCompletedDeviceNotSpecified'
            }
    , 0x20: {
              'enabled':        1
            , 'name':           'osCriticalStopShutdown'
            , 0x00:             'criticalStopduringOsLoad'
            , 0x01:             'runTimeCriticalStop'
            , 0x02:             'osGracefulStop'
            , 0x03:             'osGracefulShutdown'
            , 0x04:             'softShutdownInitiatedByPef'
            }
    , 0x21: {
              'enabled':        1
            , 'name':           'slotConnector'
            , 0x00:             'faultStatusAsserted'
            , 0x01:             'identifyStatusAsserted'
            , 0x02:             'deviceInstalledAttached'
            , 0x03:             'deviceInstallReady'
            , 0x04:             'deviceRemovalReady'
            , 0x05:             'slotPowerOff'
            , 0x05:             'slotPowerOff'
            , 0x06:             'deviceRemovalRequest'
            , 0x07:             'interlockAsserted'
            , 0x08:             'slotDisabled'
            , 0x09:             'slotHoldSpareDevice'
            }
    , 0x22: {
              'enabled':        1
            , 'name':           'systemACPIposerState'
            , 0x00:             'working'
            , 0x01:             'sleepingContextMaintained'
            , 0x02:             'sleepingProcessorContextLost'
            , 0x03:             'sleepingProcessorMemoryRetained'
            , 0x04:             'nonVolatileSleepSuspectToDisk'
            , 0x05:             'softOff'
            , 0x06:             'softOffUndetermined'
            , 0x07:             'mechanicalOff'
            , 0x08:             'sleepingUndetermined'
            , 0x09:             'sleeping'
            , 0x0a:             's5Override'
            , 0x0b:             'legacyOn'
            , 0x0c:             'legacyOff'
            , 0x0e:             'unknown'
            }
    , 0x23: {
              'enabled':        1
            , 'name':           'watchdog2'
            , 0x00:             'timerExpired'
            , 0x01:             'hardReset'
            , 0x02:             'powerDown'
            , 0x03:             'powerCycle'
            , 0x04:             'reserved0x04'
            , 0x05:             'reserved0x05'
            , 0x06:             'reserved0x06'
            , 0x07:             'reserved0x07'
            , 0x08:             'timerInterrupt'
            }
    , 0x24: {
              'enabled':        1
            , 'name':           'platformAlert'
            , 0x00:             'platformGeneratedPage'
            , 0x01:             'platformGeneratedLANalert'
            , 0x02:             'platformEventTrapGenerated'
            , 0x03:             'platformGeneratedSNMPtrapOemFormat'
            }
    , 0x25: {
              'enabled':        1
            , 'name':           'entityPresence'
            , 0x00:             'entityPresent'
            , 0x01:             'entiryAbsent'
            , 0x02:             'entiryDisabled'
            }
    , 0x26: {
              'enabled':        1
            , 'name':           'monitorASIC'
            }
    , 0x27: {
              'enabled':        1
            , 'name':           'lan'
            , 0x00:             'lanHeartbeatLost'
            , 0x01:             'lanHeartbeat'
            }
    , 0x28: {
              'enabled':        1
            , 'name':           'managementSubsystemHealth'
            , 0x00:             'sensorAccessDegradedOrUnavailable'
            , 0x01:             'controllerAccessDegradedOrUnavailable'
            , 0x02:             'managementControllerOffLine'
            , 0x03:             'managementControllerOffUnavailable'
            , 0x04:             'sensorFailure'
            , 0x05:             'fruFailure'
            , 0x01:             'lanHeartbeat'
            }
    , 0x29: {
              'enabled':        1
            , 'name':           'battery'
            , 0x00:             'batteryLow'
            , 0x01:             'batteryFailed'
            , 0x02:             'batteryePresenceDetected'
            }
    , 0x2a: {
              'enabled':        1
            , 'name':           'sessionAudit'
            , 0x00:             'sessionActivated'
            , 0x01:             'sessionDeactivated'
            , 0x02:             'invalidUsernameOrPassword'
            , 0x03:             'invalidPasswordDisable'
            }
    , 0x2b: {
              'enabled':        1
            , 'name':           'versionChange'
            , 0x00:             'hardwareChangeDetected'
            , 0x01:             'formwareOrSoftwareChangeDetected'
            , 0x02:             'hardwareIncompatible'
            , 0x03:             'FirmwareOrSoftwareIncompatible'
            , 0x04:             'entityUnsupportedHardware'
            , 0x05:             'entityUnsupportedFirmwareOrSoftware'
            , 0x06:             'hardwareChangeDetectedSucess'
            , 0x07:             'firmwareOrSoftwareChangeDetectedSucess'
            }
    , 0x2c: {
              'enabled':        1
            , 'name':           'fruState'
            , 0x00:             'fruNotInstalled'
            , 0x01:             'fruNotActive'
            , 0x02:             'fruActivationRequested'
            , 0x03:             'fruActivationInProgress'
            , 0x04:             'fruActive'
            , 0x05:             'fruDeactivationRequested'
            , 0x06:             'fruDeactivationInProgress'
            , 0x07:             'fruCOmmunicationLost'
            }
}

if __name__ == '__main__':
    print table_29_6
    print table_42_1
    print table_42_2
    print table_42_3
