#!/usr/bin/python
import os
import sys

###########################################
def file_header():
    str = """
{
  'includes': [
    '../core/core_generated.gypi',
    'modules_generated.gypi',
  ],
  'variables': {
    # Experimental hooks for embedder to provide extra IDL and source files.
    #
    # Note: this is not a supported API. If you rely on this, you will be broken
    # from time to time as the code generator changes in backward incompatible
    # ways.
    'extra_blink_module_idl_files': [],
    'extra_blink_module_files': [],
    """
    return str

###########################################end file_header

def modules_idl_files():
    str = """
    # Files for which bindings (.cpp and .h files) will be generated
    'modules_idl_files': [
      '<@(extra_blink_module_idl_files)',
    """

    str_app_banner = """
      'app_banner/AppBannerPromptResult.idl',
      'app_banner/BeforeInstallPromptEvent.idl',
    """

    str_background_sync = """
      'background_sync/PeriodicSyncEvent.idl',
      'background_sync/PeriodicSyncManager.idl',
      'background_sync/PeriodicSyncRegistration.idl',
      'background_sync/SyncEvent.idl',
      'background_sync/SyncManager.idl',
      'background_sync/SyncRegistration.idl',
    """

    str_battery = """
      'battery/BatteryManager.idl',
    """

    str_bluetooth = """
      'bluetooth/Bluetooth.idl',
      'bluetooth/BluetoothDevice.idl',
      'bluetooth/BluetoothGATTCharacteristic.idl',
      'bluetooth/BluetoothGATTRemoteServer.idl',
      'bluetooth/BluetoothGATTService.idl',
      'bluetooth/BluetoothUUID.idl',
    """

    str_cachestorage = """
      'cachestorage/Cache.idl',
      'cachestorage/CacheStorage.idl',
    """

    str_canvas2d = """
      'canvas2d/CanvasGradient.idl',
      'canvas2d/CanvasPattern.idl',
      'canvas2d/CanvasRenderingContext2D.idl',
      'canvas2d/Path2D.idl',
    """

    str_compositorworker = """
      'compositorworker/CompositorWorker.idl',
      'compositorworker/CompositorWorkerGlobalScope.idl',
    """

    str_credentialmanager = """
      'credentialmanager/Credential.idl',
      'credentialmanager/CredentialsContainer.idl',
      'credentialmanager/FederatedCredential.idl',
      'credentialmanager/PasswordCredential.idl',
    """

    str_crypto = """
      'crypto/Crypto.idl',
      'crypto/CryptoKey.idl',
      'crypto/SubtleCrypto.idl',
    """

    str_device_light = """
      'device_light/DeviceLightEvent.idl',
    """

    str_device_orientation = """
      'device_orientation/DeviceAcceleration.idl',
      'device_orientation/DeviceMotionEvent.idl',
      'device_orientation/DeviceOrientationEvent.idl',
      'device_orientation/DeviceRotationRate.idl',
    """

    str_encoding = """
      'encoding/TextDecoder.idl',
      'encoding/TextEncoder.idl',
    """

    str_encryptedmedia = """
      'encryptedmedia/MediaEncryptedEvent.idl',
      'encryptedmedia/MediaKeyMessageEvent.idl',
      'encryptedmedia/MediaKeySession.idl',
      'encryptedmedia/MediaKeyStatusMap.idl',
      'encryptedmedia/MediaKeySystemAccess.idl',
      'encryptedmedia/MediaKeys.idl',
    """

    str_fetch = """
      'fetch/Body.idl',
      'fetch/Headers.idl',
      'fetch/Request.idl',
      'fetch/Response.idl',
    """

    str_filesystem = """
      'filesystem/DOMFileSystem.idl',
      'filesystem/DOMFileSystemSync.idl',
      'filesystem/DirectoryEntry.idl',
      'filesystem/DirectoryEntrySync.idl',
      'filesystem/DirectoryReader.idl',
      'filesystem/DirectoryReaderSync.idl',
      'filesystem/EntriesCallback.idl',
      'filesystem/Entry.idl',
      'filesystem/EntryCallback.idl',
      'filesystem/EntrySync.idl',
      'filesystem/ErrorCallback.idl',
      'filesystem/FileCallback.idl',
      'filesystem/FileEntry.idl',
      'filesystem/FileEntrySync.idl',
      'filesystem/FileSystemCallback.idl',
      'filesystem/FileWriter.idl',
      'filesystem/FileWriterCallback.idl',
      'filesystem/FileWriterSync.idl',
      'filesystem/Metadata.idl',
      'filesystem/MetadataCallback.idl',
    """

    str_gamepad = """
      'gamepad/Gamepad.idl',
      'gamepad/GamepadButton.idl',
      'gamepad/GamepadEvent.idl',
      'gamepad/GamepadList.idl',
    """

    str_geo_features = """
      'geofencing/CircularGeofencingRegion.idl',
      'geofencing/Geofencing.idl',
      'geofencing/GeofencingEvent.idl',
      'geofencing/GeofencingRegion.idl',
      'geolocation/Coordinates.idl',
      'geolocation/Geolocation.idl',
      'geolocation/Geoposition.idl',
      'geolocation/PositionCallback.idl',
      'geolocation/PositionError.idl',
      'geolocation/PositionErrorCallback.idl',
    """

    str_indexeddb = """
      'indexeddb/IDBCursor.idl',
      'indexeddb/IDBCursorWithValue.idl',
      'indexeddb/IDBDatabase.idl',
      'indexeddb/IDBFactory.idl',
      'indexeddb/IDBIndex.idl',
      'indexeddb/IDBKeyRange.idl',
      'indexeddb/IDBObjectStore.idl',
      'indexeddb/IDBOpenDBRequest.idl',
      'indexeddb/IDBRequest.idl',
      'indexeddb/IDBTransaction.idl',
      'indexeddb/IDBVersionChangeEvent.idl',
    """

    str_mediasource = """
      'mediasource/MediaSource.idl',
      'mediasource/SourceBuffer.idl',
      'mediasource/SourceBufferList.idl',
      'mediasource/TrackDefault.idl',
      'mediasource/TrackDefaultList.idl',
      'mediasource/VideoPlaybackQuality.idl',
    """

    str_mediastream = """
      'mediasession/MediaSession.idl',
      'mediastream/MediaDeviceInfo.idl',
      'mediastream/MediaDevices.idl',
      'mediastream/MediaStream.idl',
      'mediastream/MediaStreamEvent.idl',
      'mediastream/MediaStreamTrack.idl',
      'mediastream/MediaStreamTrackEvent.idl',
      'mediastream/MediaStreamTrackSourcesCallback.idl',
      'mediastream/NavigatorUserMediaError.idl',
      'mediastream/NavigatorUserMediaErrorCallback.idl',
      'mediastream/NavigatorUserMediaSuccessCallback.idl',
      'mediastream/RTCDTMFSender.idl',
      'mediastream/RTCDTMFToneChangeEvent.idl',
      'mediastream/RTCDataChannel.idl',
      'mediastream/RTCDataChannelEvent.idl',
      'mediastream/RTCErrorCallback.idl',
      'mediastream/RTCIceCandidate.idl',
      'mediastream/RTCIceCandidateEvent.idl',
      'mediastream/RTCPeerConnection.idl',
      'mediastream/RTCSessionDescription.idl',
      'mediastream/RTCSessionDescriptionCallback.idl',
      'mediastream/RTCStatsCallback.idl',
      'mediastream/RTCStatsReport.idl',
      'mediastream/RTCStatsResponse.idl',
      'mediastream/SourceInfo.idl',
    """

    str_navigatorconnect = """
      'navigatorconnect/CrossOriginServiceWorkerClient.idl',
      'navigatorconnect/ServicePort.idl',
      'navigatorconnect/ServicePortConnectEvent.idl',
      'navigatorconnect/ServicePortCollection.idl',
    """

    str_netinfo = """
      'netinfo/NetworkInformation.idl',
    """

    str_nfc = """
      'nfc/NFC.idl',
    """

    str_notifications = """
      'notifications/Notification.idl',
      'notifications/NotificationEvent.idl',
      'notifications/NotificationPermissionCallback.idl',
    """

    str_permissions = """
      'permissions/Permissions.idl',
      'permissions/PermissionStatus.idl',
    """

    str_plugins = """
      'plugins/MimeType.idl',
      'plugins/MimeTypeArray.idl',
      'plugins/Plugin.idl',
      'plugins/PluginArray.idl',
    """

    str_presentation = """
      'presentation/Presentation.idl',
      'presentation/PresentationAvailability.idl',
      'presentation/PresentationRequest.idl',
      'presentation/PresentationSession.idl',
      'presentation/PresentationSessionConnectEvent.idl',
    """

    str_push_messaging = """
      'push_messaging/PushEvent.idl',
      'push_messaging/PushManager.idl',
      'push_messaging/PushMessageData.idl',
      'push_messaging/PushSubscription.idl',
    """

    str_quota = """
      'quota/DeprecatedStorageInfo.idl',
      'quota/DeprecatedStorageQuota.idl',
      'quota/StorageErrorCallback.idl',
      'quota/StorageInfo.idl',
      'quota/StorageManager.idl',
      'quota/StorageQuota.idl',
      'quota/StorageQuotaCallback.idl',
      'quota/StorageUsageCallback.idl',
    """

    str_screen_orientation = """
      'screen_orientation/ScreenOrientation.idl',
    """

    str_serviceworkers = """
      'serviceworkers/Client.idl',
      'serviceworkers/Clients.idl',
      'serviceworkers/ExtendableEvent.idl',
      'serviceworkers/FetchEvent.idl',
      'serviceworkers/ServiceWorker.idl',
      'serviceworkers/ServiceWorkerContainer.idl',
      'serviceworkers/ServiceWorkerGlobalScope.idl',
      'serviceworkers/ServiceWorkerMessageEvent.idl',
      'serviceworkers/ServiceWorkerRegistration.idl',
      'serviceworkers/WindowClient.idl',
    """

    str_speech = """
      'speech/SpeechGrammar.idl',
      'speech/SpeechGrammarList.idl',
      'speech/SpeechRecognition.idl',
      'speech/SpeechRecognitionAlternative.idl',
      'speech/SpeechRecognitionError.idl',
      'speech/SpeechRecognitionEvent.idl',
      'speech/SpeechRecognitionResult.idl',
      'speech/SpeechRecognitionResultList.idl',
      'speech/SpeechSynthesis.idl',
      'speech/SpeechSynthesisEvent.idl',
      'speech/SpeechSynthesisUtterance.idl',
      'speech/SpeechSynthesisVoice.idl',
    """

    str_storage = """
      'storage/Storage.idl',
      'storage/StorageEvent.idl',
    """

    str_vr = """
      'vr/HMDVRDevice.idl',
      'vr/PositionSensorVRDevice.idl',
      'vr/VRDevice.idl',
      'vr/VREyeParameters.idl',
      'vr/VRFieldOfView.idl',
      'vr/VRPositionState.idl',
    """

    str_webaudio = """
      'webaudio/AnalyserNode.idl',
      'webaudio/AudioBuffer.idl',
      'webaudio/AudioBufferCallback.idl',
      'webaudio/AudioBufferSourceNode.idl',
      'webaudio/AudioContext.idl',
      'webaudio/AudioDestinationNode.idl',
      'webaudio/AudioListener.idl',
      'webaudio/AudioNode.idl',
      'webaudio/AudioParam.idl',
      'webaudio/AudioProcessingEvent.idl',
      'webaudio/AudioSourceNode.idl',
      'webaudio/BiquadFilterNode.idl',
      'webaudio/ChannelMergerNode.idl',
      'webaudio/ChannelSplitterNode.idl',
      'webaudio/ConvolverNode.idl',
      'webaudio/DelayNode.idl',
      'webaudio/DynamicsCompressorNode.idl',
      'webaudio/GainNode.idl',
      'webaudio/MediaElementAudioSourceNode.idl',
      'webaudio/MediaStreamAudioDestinationNode.idl',
      'webaudio/MediaStreamAudioSourceNode.idl',
      'webaudio/OfflineAudioCompletionEvent.idl',
      'webaudio/OfflineAudioContext.idl',
      'webaudio/OscillatorNode.idl',
      'webaudio/PannerNode.idl',
      'webaudio/PeriodicWave.idl',
      'webaudio/ScriptProcessorNode.idl',
      'webaudio/StereoPannerNode.idl',
      'webaudio/WaveShaperNode.idl',
    """

    str_webcl = """
      'webcl/WebCL.idl',
      'webcl/WebCLBuffer.idl',
      'webcl/WebCLCallback.idl',
      'webcl/WebCLCommandQueue.idl',
      'webcl/WebCLContext.idl',
      'webcl/WebCLDevice.idl',
      'webcl/WebCLEvent.idl',
      'webcl/WebCLImage.idl',
      'webcl/WebCLKernel.idl',
      'webcl/WebCLKernelArgInfo.idl',
      'webcl/WebCLMemoryObject.idl',
      'webcl/WebCLPlatform.idl',
      'webcl/WebCLProgram.idl',
      'webcl/WebCLSampler.idl',
      'webcl/WebCLUserEvent.idl',
    """

    str_webdatabase = """
      'webdatabase/Database.idl',
      'webdatabase/DatabaseCallback.idl',
      'webdatabase/SQLError.idl',
      'webdatabase/SQLResultSet.idl',
      'webdatabase/SQLResultSetRowList.idl',
      'webdatabase/SQLStatementCallback.idl',
      'webdatabase/SQLStatementErrorCallback.idl',
      'webdatabase/SQLTransaction.idl',
      'webdatabase/SQLTransactionCallback.idl',
      'webdatabase/SQLTransactionErrorCallback.idl',
    """

    str_webgl = """
      'webgl/ANGLEInstancedArrays.idl',
      'webgl/CHROMIUMSubscribeUniform.idl',
      'webgl/CHROMIUMValuebuffer.idl',
      'webgl/EXTBlendMinMax.idl',
      'webgl/EXTFragDepth.idl',
      'webgl/EXTShaderTextureLOD.idl',
      'webgl/EXTTextureFilterAnisotropic.idl',
      'webgl/EXTsRGB.idl',
      'webgl/OESElementIndexUint.idl',
      'webgl/OESStandardDerivatives.idl',
      'webgl/OESTextureFloat.idl',
      'webgl/OESTextureFloatLinear.idl',
      'webgl/OESTextureHalfFloat.idl',
      'webgl/OESTextureHalfFloatLinear.idl',
      'webgl/OESVertexArrayObject.idl',
      'webgl/WebGL2RenderingContext.idl',
      'webgl/WebGLActiveInfo.idl',
      'webgl/WebGLBuffer.idl',
      'webgl/WebGLCompressedTextureATC.idl',
      'webgl/WebGLCompressedTextureETC1.idl',
      'webgl/WebGLCompressedTexturePVRTC.idl',
      'webgl/WebGLCompressedTextureS3TC.idl',
      'webgl/WebGLContextEvent.idl',
      'webgl/WebGLDebugRendererInfo.idl',
      'webgl/WebGLDebugShaders.idl',
      'webgl/WebGLDepthTexture.idl',
      'webgl/WebGLDrawBuffers.idl',
      'webgl/WebGLFramebuffer.idl',
      'webgl/WebGLLoseContext.idl',
      'webgl/WebGLProgram.idl',
      'webgl/WebGLQuery.idl',
      'webgl/WebGLRenderbuffer.idl',
      'webgl/WebGLRenderingContext.idl',
      'webgl/WebGLSampler.idl',
      'webgl/WebGLShader.idl',
      'webgl/WebGLShaderPrecisionFormat.idl',
      'webgl/WebGLSync.idl',
      'webgl/WebGLTexture.idl',
      'webgl/WebGLTransformFeedback.idl',
      'webgl/WebGLUniformLocation.idl',
      'webgl/WebGLVertexArrayObject.idl',
      'webgl/WebGLVertexArrayObjectOES.idl',
    """

    str_webmidi = """
      'webmidi/MIDIAccess.idl',
      'webmidi/MIDIConnectionEvent.idl',
      'webmidi/MIDIInput.idl',
      'webmidi/MIDIInputMap.idl',
      'webmidi/MIDIMessageEvent.idl',
      'webmidi/MIDIOutput.idl',
      'webmidi/MIDIOutputMap.idl',
      'webmidi/MIDIPort.idl',
    """

    str_websockets = """
      'websockets/CloseEvent.idl',
      'websockets/WebSocket.idl',
    """

    str_webusb = """
      'webusb/USB.idl',
      'webusb/USBAlternateInterface.idl',
      'webusb/USBEndpoint.idl',
      'webusb/USBConfiguration.idl',
      'webusb/USBConnectionEvent.idl',
      'webusb/USBDevice.idl',
      'webusb/USBInterface.idl',
    """
    str_tail = """
    ],
    """

    if not disable_app_banner:
        str = str + str_app_banner

    if not disable_background_sync:
        str = str + str_background_sync

    if not disable_battery:
        str = str + str_battery

    if not disable_bluetooth:
        str = str + str_bluetooth

    if not disable_cachestorage:
        str = str + str_cachestorage

    if not disable_canvas2d:
        str = str + str_canvas2d

    if not disable_compositorworker:
        str = str + str_compositorworker

    if not disable_credentialmanager:
        str = str + str_credentialmanager

    if not disable_crypto:
        str = str + str_crypto

    if not disable_device_light:
        str = str + str_device_light

    if not disable_device_orientation:
        str = str + str_device_orientation

    if not disable_encoding:
        str = str + str_encoding

    if not disable_encryptedmedia:
        str = str + str_encryptedmedia

    if not disable_fetch:
        str = str + str_fetch

    if not disable_filesystem:
        str = str + str_filesystem

    if not disable_gamepad:
        str = str + str_gamepad

    if not disable_geo_features:
        str = str + str_geo_features

    if not disable_indexeddb:
        str = str + str_indexeddb

    if not disable_mediasource:
        str = str + str_mediasource

    if not disable_mediastream:
        str = str + str_mediastream

    if not disable_navigatorconnect:
        str = str + str_navigatorconnect

    if not disable_netinfo:
        str = str + str_netinfo

    if not disable_nfc:
        str = str + str_nfc

    if not disable_notifications:
        str = str + str_notifications

    if not disable_permissions:
        str = str + str_permissions

    if not disable_plugins:
        str = str + str_plugins

    if not disable_presentation:
        str = str + str_presentation

    if not disable_push_messaging:
        str = str + str_push_messaging

    if not disable_quota:
        str = str + str_quota

    if not disable_screen_orientation:
        str = str + str_screen_orientation

    if not disable_serviceworkers:
        str = str + str_serviceworkers

    if not disable_speech:
        str = str + str_speech

    if not disable_storage:
        str = str + str_storage

    if not disable_vr:
        str = str + str_vr

    if not disable_webaudio:
        str = str + str_webaudio

    if not disable_webcl:
        str = str + str_webcl

    if not disable_webdatabase:
        str = str + str_webdatabase

    if not disable_webgl:
        str = str + str_webgl

    if not disable_webmidi:
        str = str + str_webmidi

    if not disable_websockets:
        str = str + str_websockets

    if not disable_webusb:
        str = str + str_webusb

    return str + str_tail
####################################end modules_idl_files


def modules_dependency_idl_files():
    str = """
    # 'partial interface' or target (right side of) 'implements'
    'modules_dependency_idl_files': [
    """

    str_background_sync = """
      'audio_output_devices/HTMLMediaElementAudioOutputDevice.idl',
      'background_sync/ServiceWorkerGlobalScopeSync.idl',
      'background_sync/ServiceWorkerRegistrationSync.idl',
    """

    str_battery = """
      'battery/NavigatorBattery.idl',
    """

    str_beacon = """
      'beacon/NavigatorBeacon.idl',
    """

    str_bluetooth = """
      'bluetooth/NavigatorBluetooth.idl',
    """

    str_cachestorage = """
      'cachestorage/WindowCacheStorage.idl',
      'cachestorage/WorkerCacheStorage.idl',
    """

    str_canvas2d = """
      'canvas2d/CanvasPathMethods.idl',
      'canvas2d/MouseEventHitRegion.idl',
      'canvas2d/TouchHitRegion.idl',
    """

    str_credentialmanager = """
      'credentialmanager/NavigatorCredentials.idl',
    """

    str_crypto = """
      'crypto/WindowCrypto.idl',
      'crypto/WorkerGlobalScopeCrypto.idl',
    """

    str_device_light = """
      'device_light/WindowDeviceLight.idl',
    """

    str_device_orientation = """
      'device_orientation/WindowDeviceMotion.idl',
      'device_orientation/WindowDeviceOrientation.idl',
    """

    str_donottrack = """
      'donottrack/NavigatorDoNotTrack.idl',
    """

    str_encryptedmedia = """
      'encryptedmedia/HTMLMediaElementEncryptedMedia.idl',
      'encryptedmedia/NavigatorRequestMediaKeySystemAccess.idl',
    """

    str_fetch = """
      'fetch/WindowFetch.idl',
      'fetch/WorkerFetch.idl',
    """

    str_filesystem = """
      'filesystem/DataTransferItemFileSystem.idl',
      'filesystem/DevToolsHostFileSystem.idl',
      'filesystem/HTMLInputElementFileSystem.idl',
      'filesystem/WindowFileSystem.idl',
      'filesystem/WorkerGlobalScopeFileSystem.idl',
    """

    str_gamepad = """
      'gamepad/NavigatorGamepad.idl',
    """

    str_geo_features = """
      'geofencing/ServiceWorkerGlobalScopeGeofencing.idl',
      'geofencing/ServiceWorkerRegistrationGeofencing.idl',
      'geofencing/WorkerNavigatorGeofencing.idl',
      'geolocation/NavigatorGeolocation.idl',
    """

    str_imagebitmap = """
      'imagebitmap/WindowImageBitmapFactories.idl',
    """

    str_indexeddb = """
      'indexeddb/WindowIndexedDatabase.idl',
      'indexeddb/WorkerGlobalScopeIndexedDatabase.idl',
    """

    str_mediasource = """
      'mediasession/HTMLMediaElementMediaSession.idl',
      'mediasource/HTMLVideoElementMediaSource.idl',
      'mediasource/URLMediaSource.idl',
    """

    str_mediastream = """
      'mediastream/NavigatorMediaStream.idl',
      'mediastream/NavigatorUserMedia.idl',
      'mediastream/URLMediaStream.idl',
      'mediastream/WindowMediaStream.idl',
    """

    str_navigatorconnect = """
      'navigatorconnect/NavigatorServices.idl',
      'navigatorconnect/ServiceWorkerGlobalScopeNavigatorConnect.idl',
      'navigatorconnect/WorkerNavigatorServices.idl',
      'navigatorcontentutils/NavigatorContentUtils.idl',
    """

    str_netinfo = """
      'netinfo/NavigatorNetworkInformation.idl',
      'netinfo/WorkerNavigatorNetworkInformation.idl',
    """

    str_nfc = """
      'nfc/NavigatorNFC.idl',
    """

    str_notifications = """
      'notifications/ServiceWorkerGlobalScopeNotifications.idl',
      'notifications/ServiceWorkerRegistrationNotifications.idl',
    """

    str_permissions = """
      'permissions/NavigatorPermissions.idl',
      'permissions/WorkerNavigatorPermissions.idl',
    """

    str_plugins = """
      'plugins/NavigatorPlugins.idl',
    """

    str_presentation = """
      'presentation/NavigatorPresentation.idl',
    """

    str_push_messaging = """
      'push_messaging/ServiceWorkerGlobalScopePush.idl',
      'push_messaging/ServiceWorkerRegistrationPush.idl',
    """

    str_quota = """
      'quota/NavigatorStorageQuota.idl',
      'quota/WindowQuota.idl',
      'quota/WorkerNavigatorStorageQuota.idl',
    """

    str_screen_orientation = """
      'screen_orientation/ScreenScreenOrientation.idl',
    """

    str_serviceworkers = """
      'serviceworkers/NavigatorServiceWorker.idl',
    """

    str_speech = """
      'speech/WindowSpeech.idl',
      'speech/WindowSpeechSynthesis.idl',
    """

    str_storage = """
      'storage/WindowStorage.idl',
    """

    str_vibration = """
      'vibration/NavigatorVibration.idl',
    """

    str_vr = """
      'vr/NavigatorVRDevice.idl',
    """

    str_wake_lock = """
      'wake_lock/ScreenWakeLock.idl',
    """

    str_webaudio = """
      'webaudio/WindowWebAudio.idl',
    """

    str_webcl = """
      'webcl/WindowWebCL.idl',
    """

    str_webdatabase = """
      'webdatabase/WindowWebDatabase.idl',
    """

    str_webgl = """
      'webgl/WebGL2RenderingContextBase.idl',
      'webgl/WebGLRenderingContextBase.idl',
    """

    str_webmidi = """
      'webmidi/NavigatorWebMIDI.idl',
    """

    str_webusb = """
      'webusb/NavigatorUSB.idl',
    """

    str_tail = """
    ],
    """

    if not disable_background_sync:
        str = str + str_background_sync

    if not disable_battery:
        str = str + str_battery

    if not disable_beacon:
        str = str + str_beacon

    if not disable_bluetooth:
        str = str + str_bluetooth

    if not disable_cachestorage:
        str = str + str_cachestorage

    if not disable_canvas2d:
        str = str + str_canvas2d

    if not disable_credentialmanager:
        str = str + str_credentialmanager

    if not disable_crypto:
        str = str + str_crypto

    if not disable_device_light:
        str = str + str_device_light

    if not disable_device_orientation:
        str = str + str_device_orientation

    if not disable_donottrack:
        str = str + str_donottrack

    if not disable_encryptedmedia:
        str = str + str_encryptedmedia

    if not disable_fetch:
        str = str + str_fetch

    if not disable_filesystem:
        str = str + str_filesystem

    if not disable_gamepad:
        str = str + str_gamepad

    if not disable_geo_features:
        str = str + str_geo_features

    str = str + str_imagebitmap

    if not disable_indexeddb:
        str = str + str_indexeddb

    if not disable_mediasource:
        str = str + str_mediasource

    if not disable_mediastream:
        str = str + str_mediastream

    if not disable_navigatorconnect:
        str = str + str_navigatorconnect

    if not disable_netinfo:
        str = str + str_netinfo

    if not disable_nfc:
        str = str + str_nfc

    if not disable_notifications:
        str = str + str_notifications

    if not disable_permissions:
        str = str + str_permissions

    if not disable_plugins:
        str = str + str_plugins

    if not disable_presentation:
        str = str + str_presentation

    if not disable_push_messaging:
        str = str + str_push_messaging

    if not disable_quota:
        str = str + str_quota

    if not disable_screen_orientation:
        str = str + str_screen_orientation

    if not disable_serviceworkers:
        str = str + str_serviceworkers

    if not disable_speech:
        str = str + str_speech

    if not disable_storage:
        str = str + str_storage

    if not disable_vibration:
        str = str + str_vibration

    if not disable_vr:
        str = str + str_vr

    str = str + str_wake_lock

    if not disable_webaudio:
        str = str + str_webaudio

    if not disable_webcl:
        str = str + str_webcl

    if not disable_webdatabase:
        str = str + str_webdatabase

    if not disable_webgl:
        str = str + str_webgl

    if not disable_webmidi:
        str = str + str_webmidi

    if not disable_webusb:
        str = str + str_webusb

    return str + str_tail
####################################end modules_dependency_idl_files

def modules_event_idl_files():
    str = """
    # interfaces that inherit from Event
    'modules_event_idl_files': [
    """

    str_app_banner = """
      'app_banner/BeforeInstallPromptEvent.idl',
    """

    str_background_sync = """
      'background_sync/PeriodicSyncEvent.idl',
      'background_sync/SyncEvent.idl',
    """

    str_device_light = """
      'device_light/DeviceLightEvent.idl',
    """

    str_device_orientation = """
      'device_orientation/DeviceMotionEvent.idl',
      'device_orientation/DeviceOrientationEvent.idl',
    """

    str_encryptedmedia = """
      'encryptedmedia/MediaEncryptedEvent.idl',
      'encryptedmedia/MediaKeyMessageEvent.idl',
    """

    str_gamepad = """
      'gamepad/GamepadEvent.idl',
    """

    str_geo_features = """
      'geofencing/GeofencingEvent.idl',
    """

    str_indexeddb = """
      'indexeddb/IDBVersionChangeEvent.idl',
    """

    str_mediastream = """
      'mediastream/MediaStreamEvent.idl',
      'mediastream/MediaStreamTrackEvent.idl',
      'mediastream/RTCDTMFToneChangeEvent.idl',
      'mediastream/RTCDataChannelEvent.idl',
      'mediastream/RTCIceCandidateEvent.idl',
    """

    str_navigatorconnect = """
      'navigatorconnect/ServicePortConnectEvent.idl',
    """

    str_notifications = """
      'notifications/NotificationEvent.idl',
    """

    str_presentation = """
      'presentation/PresentationSessionConnectEvent.idl',
    """

    str_push_messaging = """
      'push_messaging/PushEvent.idl',
    """

    str_serviceworkers = """
      'serviceworkers/ExtendableEvent.idl',
      'serviceworkers/FetchEvent.idl',
      'serviceworkers/ServiceWorkerMessageEvent.idl',
    """

    str_speech = """
      'speech/SpeechRecognitionError.idl',
      'speech/SpeechRecognitionEvent.idl',
      'speech/SpeechSynthesisEvent.idl',
    """

    str_storage = """
      'storage/StorageEvent.idl',
    """

    str_webaudio = """
      'webaudio/AudioProcessingEvent.idl',
      'webaudio/OfflineAudioCompletionEvent.idl',
    """

    str_webgl = """
      'webgl/WebGLContextEvent.idl',
    """

    str_webmidi = """
      'webmidi/MIDIConnectionEvent.idl',
      'webmidi/MIDIMessageEvent.idl',
    """

    str_websockets = """
      'websockets/CloseEvent.idl',
    """

    str_tail = """
    ],
    """

    if not disable_app_banner:
        str = str + str_app_banner

    if not disable_background_sync:
        str = str + str_background_sync

    if not disable_device_light:
        str = str + str_device_light

    if not disable_device_orientation:
        str = str + str_device_orientation

    if not disable_encryptedmedia:
        str = str + str_encryptedmedia

    if not disable_gamepad:
        str = str + str_gamepad

    if not disable_geo_features:
        str = str + str_geo_features

    if not disable_indexeddb:
        str = str + str_indexeddb;

    if not disable_mediastream:
        str = str + str_mediastream;

    if not disable_navigatorconnect:
        str = str + str_navigatorconnect

    if not disable_notifications:
        str = str + str_notifications

    if not disable_presentation:
        str = str + str_presentation

    if not disable_push_messaging:
        str = str + str_push_messaging

    if not disable_serviceworkers:
        str = str + str_serviceworkers

    if not disable_speech:
        str = str + str_speech

    if not disable_storage:
        str = str + str_storage

    if not disable_webaudio:
        str = str + str_webaudio

    if not disable_webgl:
        str = str + str_webgl

    if not disable_webmidi:
        str = str + str_webmidi

    if not disable_websockets:
        str = str + str_websockets

    return str + str_tail
####################################end modules_event_idl_files


def modules_dictionary_idl_files():
    str = """
    'modules_dictionary_idl_files': [
    """

    str_app_banner = """
      'app_banner/BeforeInstallPromptEventInit.idl',
    """

    str_background_sync = """
      'background_sync/PeriodicSyncEventInit.idl',
      'background_sync/PeriodicSyncRegistrationOptions.idl',
      'background_sync/SyncEventInit.idl',
      'background_sync/SyncRegistrationOptions.idl',
    """

    str_bluetooth = """
      'bluetooth/BluetoothScanFilter.idl',
      'bluetooth/RequestDeviceOptions.idl',
    """

    str_cachestorage = """
      'cachestorage/CacheQueryOptions.idl',
    """

    str_canvas2d = """
      'canvas2d/Canvas2DContextAttributes.idl',
      'canvas2d/HitRegionOptions.idl',
    """

    str_credentialmanager = """
      'credentialmanager/CredentialData.idl',
      'credentialmanager/CredentialRequestOptions.idl',
      'credentialmanager/FederatedCredentialData.idl',
      'credentialmanager/FederatedCredentialRequestOptions.idl',
      'credentialmanager/LocallyStoredCredentialData.idl',
      'credentialmanager/PasswordCredentialData.idl',
    """

    str_device_light = """
      'device_light/DeviceLightEventInit.idl',
    """

    str_encoding = """
      'encoding/TextDecodeOptions.idl',
      'encoding/TextDecoderOptions.idl',
    """

    str_encryptedmedia = """
      'encryptedmedia/MediaEncryptedEventInit.idl',
      'encryptedmedia/MediaKeyMessageEventInit.idl',
      'encryptedmedia/MediaKeySystemConfiguration.idl',
      'encryptedmedia/MediaKeySystemMediaCapability.idl',
    """

    str_filesystem = """
      'filesystem/FileSystemFlags.idl',
    """

    str_gamepad = """
      'gamepad/GamepadEventInit.idl',
    """

    str_geo_features = """
      'geofencing/CircularGeofencingRegionInit.idl',
      'geolocation/PositionOptions.idl',
    """

    str_indexeddb = """
      'indexeddb/IDBIndexParameters.idl',
      'indexeddb/IDBObjectStoreParameters.idl',
      'indexeddb/IDBVersionChangeEventInit.idl',
    """

    str_mediastream = """
      'mediastream/MediaStreamConstraints.idl',
      'mediastream/MediaStreamEventInit.idl',
      'mediastream/RTCDTMFToneChangeEventInit.idl',
      'mediastream/RTCIceCandidateInit.idl',
      'mediastream/RTCSessionDescriptionInit.idl',
    """

    str_navigatorconnect = """
      'navigatorconnect/ServicePortConnectEventInit.idl',
      'navigatorconnect/ServicePortConnectOptions.idl',
      'navigatorconnect/ServicePortConnectResponse.idl',
      'navigatorconnect/ServicePortMatchOptions.idl',
    """

    str_notifications = """
      'notifications/GetNotificationOptions.idl',
      'notifications/NotificationAction.idl',
      'notifications/NotificationEventInit.idl',
      'notifications/NotificationOptions.idl',
    """

    str_permissions = """
      'permissions/MidiPermissionDescriptor.idl',
      'permissions/PermissionDescriptor.idl',
      'permissions/PushPermissionDescriptor.idl',
    """

    str_presentation = """
      'presentation/PresentationSessionConnectEventInit.idl',
    """

    str_push_messaging = """
      'push_messaging/PushEventInit.idl',
      'push_messaging/PushSubscriptionOptions.idl',
    """

    str_serviceworkers = """
      'serviceworkers/ClientQueryOptions.idl',
      'serviceworkers/ExtendableEventInit.idl',
      'serviceworkers/FetchEventInit.idl',
      'serviceworkers/RegistrationOptions.idl',
      'serviceworkers/ServiceWorkerMessageEventInit.idl',
    """

    str_speech = """
      'speech/SpeechRecognitionErrorInit.idl',
      'speech/SpeechRecognitionEventInit.idl',
    """

    str_storage = """
      'storage/StorageEventInit.idl',
    """

    str_vr = """
      'vr/VRFieldOfViewInit.idl',
    """

    str_webcl = """
      'webcl/WebCLImageDescriptor.idl',
    """

    str_webgl = """
      'webgl/WebGLContextAttributes.idl',
      'webgl/WebGLContextEventInit.idl',
    """

    str_webmidi = """
      'webmidi/MIDIConnectionEventInit.idl',
      'webmidi/MIDIMessageEventInit.idl',
      'webmidi/MIDIOptions.idl',
    """

    str_websockets = """
      'websockets/CloseEventInit.idl',
    """

    str_webusb = """
      'webusb/USBConnectionEventInit.idl',
      'webusb/USBDeviceRequestOptions.idl',
      'webusb/USBDeviceFilter.idl',
    """

    str_tail = """
    ],
    """

    if not disable_app_banner:
        str = str + str_app_banner

    if not disable_background_sync:
        str = str + str_background_sync

    if not disable_bluetooth:
        str = str + str_bluetooth

    if not disable_cachestorage:
        str = str + str_cachestorage

    if not disable_canvas2d:
        str = str + str_canvas2d

    str = str + str_credentialmanager

    if not disable_device_light:
        str = str + str_device_light

    if not disable_encoding:
        str = str + str_encoding

    if not disable_encryptedmedia:
        str = str + str_encryptedmedia

    if not disable_filesystem:
        str = str + str_filesystem

    if not disable_gamepad:
        str = str + str_gamepad

    if not disable_geo_features:
        str = str + str_geo_features

    if not disable_indexeddb:
        str = str + str_indexeddb;

    if not disable_mediastream:
        str = str + str_mediastream;

    if not disable_navigatorconnect:
        str = str + str_navigatorconnect

    if not disable_notifications:
        str = str + str_notifications

    if not disable_permissions:
        str = str + str_permissions

    if not disable_presentation:
        str = str + str_presentation

    if not disable_push_messaging:
        str = str + str_push_messaging

    if not disable_serviceworkers:
        str = str + str_serviceworkers

    if not disable_speech:
        str = str + str_speech

    if not disable_storage:
        str = str + str_storage

    if not disable_vr:
        str = str + str_vr

    if not disable_webcl:
        str = str + str_webcl

    if not disable_webgl:
        str = str + str_webgl

    if not disable_webmidi:
        str = str + str_webmidi

    if not disable_websockets:
        str = str + str_websockets

    if not disable_webusb:
        str = str + str_webusb

    return str + str_tail
####################################end modules_dictionary_idl_files

def generated_modules_files():
    str = """
    'generated_modules_files': [
      # .cpp files from make_modules_generated actions.
      '<(blink_modules_output_dir)/EventModules.cpp',
      '<(blink_modules_output_dir)/EventModulesHeaders.h',
      '<(blink_modules_output_dir)/EventModulesNames.cpp',
      '<(blink_modules_output_dir)/EventModulesNames.h',
      '<(blink_modules_output_dir)/EventTargetModulesNames.cpp',
      '<(blink_modules_output_dir)/EventTargetModulesNames.h',
      '<(blink_modules_output_dir)/IndexedDBNames.cpp',
      '<(blink_modules_output_dir)/IndexedDBNames.h',
    ],
    """

    return str
####################################end generated_modules_files

def generated_modules_dictionary_files():
    str = """
    'generated_modules_dictionary_files': [
    """

    str_app_banner = """
      '<(blink_modules_output_dir)/app_banner/BeforeInstallPromptEventInit.cpp',
      '<(blink_modules_output_dir)/app_banner/BeforeInstallPromptEventInit.h',
    """

    str_background_sync = """
      '<(blink_modules_output_dir)/background_sync/PeriodicSyncEventInit.cpp',
      '<(blink_modules_output_dir)/background_sync/PeriodicSyncEventInit.h',
      '<(blink_modules_output_dir)/background_sync/PeriodicSyncRegistrationOptions.cpp',
      '<(blink_modules_output_dir)/background_sync/PeriodicSyncRegistrationOptions.h',
      '<(blink_modules_output_dir)/background_sync/SyncEventInit.cpp',
      '<(blink_modules_output_dir)/background_sync/SyncEventInit.h',
      '<(blink_modules_output_dir)/background_sync/SyncRegistrationOptions.cpp',
      '<(blink_modules_output_dir)/background_sync/SyncRegistrationOptions.h',
    """

    str_bluetooth = """
      '<(blink_modules_output_dir)/bluetooth/BluetoothScanFilter.cpp',
      '<(blink_modules_output_dir)/bluetooth/RequestDeviceOptions.cpp',
    """

    str_cachestorage = """
      '<(blink_modules_output_dir)/cachestorage/CacheQueryOptions.cpp',
      '<(blink_modules_output_dir)/cachestorage/CacheQueryOptions.h',
    """

    str_canvas2d = """
      '<(blink_modules_output_dir)/canvas2d/Canvas2DContextAttributes.cpp',
      '<(blink_modules_output_dir)/canvas2d/Canvas2DContextAttributes.h',
      '<(blink_modules_output_dir)/canvas2d/HitRegionOptions.cpp',
      '<(blink_modules_output_dir)/canvas2d/HitRegionOptions.h',
      '<(blink_modules_output_dir)/credentialmanager/CredentialData.cpp',
      '<(blink_modules_output_dir)/credentialmanager/CredentialData.h',
      '<(blink_modules_output_dir)/credentialmanager/CredentialRequestOptions.cpp',
      '<(blink_modules_output_dir)/credentialmanager/CredentialRequestOptions.h',
      '<(blink_modules_output_dir)/credentialmanager/FederatedCredentialData.cpp',
      '<(blink_modules_output_dir)/credentialmanager/FederatedCredentialData.h',
      '<(blink_modules_output_dir)/credentialmanager/FederatedCredentialRequestOptions.cpp',
      '<(blink_modules_output_dir)/credentialmanager/FederatedCredentialRequestOptions.h',
      '<(blink_modules_output_dir)/credentialmanager/LocallyStoredCredentialData.cpp',
      '<(blink_modules_output_dir)/credentialmanager/LocallyStoredCredentialData.h',
      '<(blink_modules_output_dir)/credentialmanager/PasswordCredentialData.cpp',
      '<(blink_modules_output_dir)/credentialmanager/PasswordCredentialData.h',
    """

    str_device_light = """
      '<(blink_modules_output_dir)/device_light/DeviceLightEventInit.cpp',
      '<(blink_modules_output_dir)/device_light/DeviceLightEventInit.h',
    """

    str_encoding = """
      '<(blink_modules_output_dir)/encoding/TextDecodeOptions.cpp',
      '<(blink_modules_output_dir)/encoding/TextDecodeOptions.h',
      '<(blink_modules_output_dir)/encoding/TextDecoderOptions.cpp',
      '<(blink_modules_output_dir)/encoding/TextDecoderOptions.h',
    """

    str_encryptedmedia = """
      '<(blink_modules_output_dir)/encryptedmedia/MediaEncryptedEventInit.cpp',
      '<(blink_modules_output_dir)/encryptedmedia/MediaEncryptedEventInit.h',
      '<(blink_modules_output_dir)/encryptedmedia/MediaKeyMessageEventInit.cpp',
      '<(blink_modules_output_dir)/encryptedmedia/MediaKeyMessageEventInit.h',
      '<(blink_modules_output_dir)/encryptedmedia/MediaKeySystemConfiguration.cpp',
      '<(blink_modules_output_dir)/encryptedmedia/MediaKeySystemConfiguration.h',
      '<(blink_modules_output_dir)/encryptedmedia/MediaKeySystemMediaCapability.cpp',
      '<(blink_modules_output_dir)/encryptedmedia/MediaKeySystemMediaCapability.h',
    """

    str_filesystem = """
      '<(blink_modules_output_dir)/filesystem/FileSystemFlags.cpp',
      '<(blink_modules_output_dir)/filesystem/FileSystemFlags.h',
    """

    str_gamepad = """
      '<(blink_modules_output_dir)/gamepad/GamepadEventInit.cpp',
      '<(blink_modules_output_dir)/gamepad/GamepadEventInit.h',
    """

    str_geo_features = """
      '<(blink_modules_output_dir)/geofencing/CircularGeofencingRegionInit.cpp',
      '<(blink_modules_output_dir)/geofencing/CircularGeofencingRegionInit.h',
      '<(blink_modules_output_dir)/geolocation/PositionOptions.cpp',
      '<(blink_modules_output_dir)/geolocation/PositionOptions.h',
    """

    str_indexeddb = """
      '<(blink_modules_output_dir)/indexeddb/IDBIndexParameters.cpp',
      '<(blink_modules_output_dir)/indexeddb/IDBIndexParameters.h',
      '<(blink_modules_output_dir)/indexeddb/IDBObjectStoreParameters.cpp',
      '<(blink_modules_output_dir)/indexeddb/IDBObjectStoreParameters.h',
      '<(blink_modules_output_dir)/indexeddb/IDBVersionChangeEventInit.cpp',
      '<(blink_modules_output_dir)/indexeddb/IDBVersionChangeEventInit.h',
    """

    str_mediastream = """
      '<(blink_modules_output_dir)/mediastream/MediaStreamConstraints.cpp',
      '<(blink_modules_output_dir)/mediastream/MediaStreamConstraints.h',
      '<(blink_modules_output_dir)/mediastream/MediaStreamEventInit.cpp',
      '<(blink_modules_output_dir)/mediastream/MediaStreamEventInit.h',
      '<(blink_modules_output_dir)/mediastream/RTCDTMFToneChangeEventInit.cpp',
      '<(blink_modules_output_dir)/mediastream/RTCDTMFToneChangeEventInit.h',
      '<(blink_modules_output_dir)/mediastream/RTCIceCandidateInit.cpp',
      '<(blink_modules_output_dir)/mediastream/RTCIceCandidateInit.h',
      '<(blink_modules_output_dir)/mediastream/RTCSessionDescriptionInit.cpp',
      '<(blink_modules_output_dir)/mediastream/RTCSessionDescriptionInit.h',
    """

    str_navigatorconnect = """
      '<(blink_modules_output_dir)/navigatorconnect/ServicePortConnectEventInit.cpp',
      '<(blink_modules_output_dir)/navigatorconnect/ServicePortConnectEventInit.h',
      '<(blink_modules_output_dir)/navigatorconnect/ServicePortConnectOptions.cpp',
      '<(blink_modules_output_dir)/navigatorconnect/ServicePortConnectOptions.h',
      '<(blink_modules_output_dir)/navigatorconnect/ServicePortConnectResponse.cpp',
      '<(blink_modules_output_dir)/navigatorconnect/ServicePortConnectResponse.h',
      '<(blink_modules_output_dir)/navigatorconnect/ServicePortMatchOptions.cpp',
      '<(blink_modules_output_dir)/navigatorconnect/ServicePortMatchOptions.h',
    """

    str_notifications = """
      '<(blink_modules_output_dir)/notifications/GetNotificationOptions.cpp',
      '<(blink_modules_output_dir)/notifications/GetNotificationOptions.h',
      '<(blink_modules_output_dir)/notifications/NotificationAction.cpp',
      '<(blink_modules_output_dir)/notifications/NotificationAction.h',
      '<(blink_modules_output_dir)/notifications/NotificationEventInit.cpp',
      '<(blink_modules_output_dir)/notifications/NotificationEventInit.h',
      '<(blink_modules_output_dir)/notifications/NotificationOptions.cpp',
      '<(blink_modules_output_dir)/notifications/NotificationOptions.h',
    """

    str_permissions = """
      '<(blink_modules_output_dir)/permissions/MidiPermissionDescriptor.cpp',
      '<(blink_modules_output_dir)/permissions/MidiPermissionDescriptor.h',
      '<(blink_modules_output_dir)/permissions/PermissionDescriptor.cpp',
      '<(blink_modules_output_dir)/permissions/PermissionDescriptor.h',
      '<(blink_modules_output_dir)/permissions/PushPermissionDescriptor.cpp',
      '<(blink_modules_output_dir)/permissions/PushPermissionDescriptor.h',
    """

    str_presentation = """
      '<(blink_modules_output_dir)/presentation/PresentationSessionConnectEventInit.cpp',
      '<(blink_modules_output_dir)/presentation/PresentationSessionConnectEventInit.h',
    """

    str_push_messaging = """
      '<(blink_modules_output_dir)/push_messaging/PushEventInit.cpp',
      '<(blink_modules_output_dir)/push_messaging/PushEventInit.h',
      '<(blink_modules_output_dir)/push_messaging/PushSubscriptionOptions.cpp',
      '<(blink_modules_output_dir)/push_messaging/PushSubscriptionOptions.h',
    """

    str_serviceworkers = """
      '<(blink_modules_output_dir)/serviceworkers/ClientQueryOptions.cpp',
      '<(blink_modules_output_dir)/serviceworkers/ClientQueryOptions.h',
      '<(blink_modules_output_dir)/serviceworkers/ExtendableEventInit.cpp',
      '<(blink_modules_output_dir)/serviceworkers/ExtendableEventInit.h',
      '<(blink_modules_output_dir)/serviceworkers/FetchEventInit.cpp',
      '<(blink_modules_output_dir)/serviceworkers/FetchEventInit.h',
      '<(blink_modules_output_dir)/serviceworkers/RegistrationOptions.cpp',
      '<(blink_modules_output_dir)/serviceworkers/RegistrationOptions.h',
      '<(blink_modules_output_dir)/serviceworkers/ServiceWorkerMessageEventInit.cpp',
      '<(blink_modules_output_dir)/serviceworkers/ServiceWorkerMessageEventInit.h',
    """

    str_speech = """
      '<(blink_modules_output_dir)/speech/SpeechRecognitionErrorInit.cpp',
      '<(blink_modules_output_dir)/speech/SpeechRecognitionErrorInit.h',
      '<(blink_modules_output_dir)/speech/SpeechRecognitionEventInit.cpp',
      '<(blink_modules_output_dir)/speech/SpeechRecognitionEventInit.h',
    """

    str_storage = """
      '<(blink_modules_output_dir)/storage/StorageEventInit.cpp',
      '<(blink_modules_output_dir)/storage/StorageEventInit.h',
    """

    str_vr = """
      '<(blink_modules_output_dir)/vr/VRFieldOfViewInit.cpp',
      '<(blink_modules_output_dir)/vr/VRFieldOfViewInit.h',
    """

    str_webcl = """
      '<(blink_modules_output_dir)/webcl/WebCLImageDescriptor.cpp',
      '<(blink_modules_output_dir)/webcl/WebCLImageDescriptor.h',
    """

    str_webgl = """
      '<(blink_modules_output_dir)/webgl/WebGLContextAttributes.cpp',
      '<(blink_modules_output_dir)/webgl/WebGLContextAttributes.h',
      '<(blink_modules_output_dir)/webgl/WebGLContextEventInit.cpp',
      '<(blink_modules_output_dir)/webgl/WebGLContextEventInit.h',
    """

    str_webmidi = """
      '<(blink_modules_output_dir)/webmidi/MIDIConnectionEventInit.cpp',
      '<(blink_modules_output_dir)/webmidi/MIDIConnectionEventInit.h',
      '<(blink_modules_output_dir)/webmidi/MIDIMessageEventInit.h',
      '<(blink_modules_output_dir)/webmidi/MIDIMessageEventInit.cpp',
      '<(blink_modules_output_dir)/webmidi/MIDIOptions.cpp',
      '<(blink_modules_output_dir)/webmidi/MIDIOptions.h',
    """

    str_websockets = """
      '<(blink_modules_output_dir)/websockets/CloseEventInit.cpp',
      '<(blink_modules_output_dir)/websockets/CloseEventInit.h',
    """

    str_webusb = """
      '<(blink_modules_output_dir)/webusb/USBConnectionEventInit.cpp',
      '<(blink_modules_output_dir)/webusb/USBConnectionEventInit.h',
      '<(blink_modules_output_dir)/webusb/USBDeviceFilter.cpp',
      '<(blink_modules_output_dir)/webusb/USBDeviceFilter.h',
      '<(blink_modules_output_dir)/webusb/USBDeviceRequestOptions.cpp',
      '<(blink_modules_output_dir)/webusb/USBDeviceRequestOptions.h',
    """

    str_tail = """
    ],
    """

    if not disable_app_banner:
        str = str + str_app_banner

    if not disable_background_sync:
        str = str + str_background_sync

    if not disable_bluetooth:
        str = str + str_bluetooth

    if not disable_cachestorage:
        str = str + str_cachestorage

    if not disable_canvas2d:
        str = str + str_canvas2d

    if not disable_device_light:
        str = str + str_device_light

    if not disable_encoding:
        str = str + str_encoding

    if not disable_encryptedmedia:
        str = str + str_encryptedmedia

    if not disable_filesystem:
        str = str + str_filesystem

    if not disable_gamepad:
        str = str + str_gamepad

    if not disable_geo_features:
        str = str + str_geo_features

    if not disable_indexeddb:
        str = str + str_indexeddb

    if not disable_mediastream:
        str = str + str_mediastream

    if not disable_navigatorconnect:
        str = str + str_navigatorconnect

    if not disable_notifications:
        str = str + str_notifications

    if not disable_permissions:
        str = str + str_permissions

    if not disable_presentation:
        str = str + str_presentation

    if not disable_push_messaging:
        str = str + str_push_messaging

    if not disable_serviceworkers:
        str = str + str_serviceworkers

    if not disable_speech:
        str = str + str_speech

    if not disable_storage:
        str = str + str_storage

    if not disable_vr:
        str = str + str_vr

    if not disable_webcl:
        str = str + str_webcl

    if not disable_webgl:
        str = str + str_webgl

    if not disable_webmidi:
        str = str + str_webmidi

    if not disable_websockets:
        str = str + str_websockets

    if not disable_webusb:
        str = str + str_webusb

    return str + str_tail
####################################end generated_modules_dictionary_files

def modules_files():
    str = """
    'modules_files': [
      '<@(extra_blink_module_files)',
      '<@(generated_modules_dictionary_files)',
      '<@(generated_modules_files)',
      'InitModules.cpp',
      'InitModules.h',
    """

    str_accessibility = """
      'accessibility/AXARIAGrid.cpp',
      'accessibility/AXARIAGrid.h',
      'accessibility/AXARIAGridCell.cpp',
      'accessibility/AXARIAGridCell.h',
      'accessibility/AXARIAGridRow.cpp',
      'accessibility/AXARIAGridRow.h',
      'accessibility/AXImageMapLink.cpp',
      'accessibility/AXImageMapLink.h',
      'accessibility/AXInlineTextBox.cpp',
      'accessibility/AXInlineTextBox.h',
      'accessibility/AXLayoutObject.cpp',
      'accessibility/AXLayoutObject.h',
      'accessibility/AXList.cpp',
      'accessibility/AXList.h',
      'accessibility/AXListBox.cpp',
      'accessibility/AXListBox.h',
      'accessibility/AXListBoxOption.cpp',
      'accessibility/AXListBoxOption.h',
      'accessibility/AXMediaControls.cpp',
      'accessibility/AXMediaControls.h',
      'accessibility/AXMenuList.cpp',
      'accessibility/AXMenuList.h',
      'accessibility/AXMenuListOption.cpp',
      'accessibility/AXMenuListOption.h',
      'accessibility/AXMenuListPopup.cpp',
      'accessibility/AXMenuListPopup.h',
      'accessibility/AXMockObject.cpp',
      'accessibility/AXMockObject.h',
      'accessibility/AXNodeObject.cpp',
      'accessibility/AXNodeObject.h',
      'accessibility/AXObjectCacheImpl.cpp',
      'accessibility/AXObjectCacheImpl.h',
      'accessibility/AXObject.cpp',
      'accessibility/AXObject.h',
      'accessibility/AXProgressIndicator.cpp',
      'accessibility/AXProgressIndicator.h',
      'accessibility/AXSVGRoot.cpp',
      'accessibility/AXSVGRoot.h',
      'accessibility/AXScrollView.cpp',
      'accessibility/AXScrollView.h',
      'accessibility/AXScrollbar.cpp',
      'accessibility/AXScrollbar.h',
      'accessibility/AXSlider.cpp',
      'accessibility/AXSlider.h',
      'accessibility/AXSpinButton.cpp',
      'accessibility/AXSpinButton.h',
      'accessibility/AXTable.cpp',
      'accessibility/AXTable.h',
      'accessibility/AXTableCell.cpp',
      'accessibility/AXTableCell.h',
      'accessibility/AXTableColumn.cpp',
      'accessibility/AXTableColumn.h',
      'accessibility/AXTableHeaderContainer.cpp',
      'accessibility/AXTableHeaderContainer.h',
      'accessibility/AXTableRow.cpp',
      'accessibility/AXTableRow.h',
      'accessibility/InspectorAccessibilityAgent.cpp',
      'accessibility/InspectorAccessibilityAgent.h',
      'accessibility/InspectorTypeBuilderHelper.cpp',
      'accessibility/InspectorTypeBuilderHelper.h',
    """

    str_app_banner = """
      'app_banner/AppBannerCallbacks.cpp',
      'app_banner/AppBannerCallbacks.h',
      'app_banner/AppBannerController.cpp',
      'app_banner/AppBannerController.h',
      'app_banner/AppBannerPromptResult.cpp',
      'app_banner/AppBannerPromptResult.h',
      'app_banner/BeforeInstallPromptEvent.cpp',
      'app_banner/BeforeInstallPromptEvent.h',
    """

    str_audio_output = """
      'audio_output_devices/HTMLMediaElementAudioOutputDevice.cpp',
      'audio_output_devices/HTMLMediaElementAudioOutputDevice.h',
      'audio_output_devices/SetSinkIdCallbacks.cpp',
      'audio_output_devices/SetSinkIdCallbacks.h',
    """

    str_background_sync = """
      'background_sync/PeriodicSyncEvent.cpp',
      'background_sync/PeriodicSyncEvent.h',
      'background_sync/PeriodicSyncManager.cpp',
      'background_sync/PeriodicSyncManager.h',
      'background_sync/PeriodicSyncRegistration.cpp',
      'background_sync/PeriodicSyncRegistration.h',
      'background_sync/ServiceWorkerGlobalScopeSync.h',
      'background_sync/ServiceWorkerRegistrationSync.cpp',
      'background_sync/ServiceWorkerRegistrationSync.h',
      'background_sync/SyncCallbacks.cpp',
      'background_sync/SyncCallbacks.h',
      'background_sync/SyncError.cpp',
      'background_sync/SyncError.h',
      'background_sync/SyncEvent.cpp',
      'background_sync/SyncEvent.h',
      'background_sync/SyncManager.cpp',
      'background_sync/SyncManager.h',
      'background_sync/SyncRegistration.cpp',
      'background_sync/SyncRegistration.h',
    """

    str_battery = """
      'battery/BatteryDispatcher.cpp',
      'battery/BatteryDispatcher.h',
      'battery/BatteryManager.cpp',
      'battery/BatteryManager.h',
      'battery/BatteryStatus.cpp',
      'battery/BatteryStatus.h',
      'battery/NavigatorBattery.cpp',
      'battery/NavigatorBattery.h',
    """

    str_beacon = """
      'beacon/NavigatorBeacon.cpp',
      'beacon/NavigatorBeacon.h',
    """

    str_bluetooth = """
      'bluetooth/Bluetooth.h',
      'bluetooth/ConvertWebVectorToArrayBuffer.cpp',
      'bluetooth/ConvertWebVectorToArrayBuffer.h',
      'bluetooth/Bluetooth.cpp',
      'bluetooth/BluetoothDevice.cpp',
      'bluetooth/BluetoothDevice.h',
      'bluetooth/BluetoothError.cpp',
      'bluetooth/BluetoothError.h',
      'bluetooth/BluetoothGATTCharacteristic.cpp',
      'bluetooth/BluetoothGATTCharacteristic.h',
      'bluetooth/BluetoothGATTRemoteServer.cpp',
      'bluetooth/BluetoothGATTRemoteServer.h',
      'bluetooth/BluetoothGATTService.cpp',
      'bluetooth/BluetoothGATTService.h',
      'bluetooth/BluetoothSupplement.cpp',
      'bluetooth/BluetoothSupplement.h',
      'bluetooth/BluetoothUUID.cpp',
      'bluetooth/BluetoothUUID.h',
      'bluetooth/NavigatorBluetooth.cpp',
      'bluetooth/NavigatorBluetooth.h',
    """

    str_cachestorage = """
      'cachestorage/Cache.cpp',
      'cachestorage/Cache.h',
      'cachestorage/CacheStorage.cpp',
      'cachestorage/CacheStorage.h',
      'cachestorage/CacheStorageError.cpp',
      'cachestorage/CacheStorageError.h',
      'cachestorage/GlobalCacheStorage.cpp',
      'cachestorage/GlobalCacheStorage.h',
      'cachestorage/InspectorCacheStorageAgent.cpp',
      'cachestorage/InspectorCacheStorageAgent.h',
    """

    str_canvas2d = """
      'canvas2d/CanvasGradient.cpp',
      'canvas2d/CanvasGradient.h',
      'canvas2d/CanvasPathMethods.cpp',
      'canvas2d/CanvasPathMethods.h',
      'canvas2d/CanvasPattern.cpp',
      'canvas2d/CanvasPattern.h',
      'canvas2d/CanvasRenderingContext2D.cpp',
      'canvas2d/CanvasRenderingContext2D.h',
      'canvas2d/CanvasRenderingContext2DState.cpp',
      'canvas2d/CanvasRenderingContext2DState.h',
      'canvas2d/CanvasStyle.cpp',
      'canvas2d/CanvasStyle.h',
      'canvas2d/ClipList.cpp',
      'canvas2d/ClipList.h',
      'canvas2d/ContextAttributeHelpers.cpp',
      'canvas2d/ContextAttributeHelpers.h',
      'canvas2d/EventHitRegion.cpp',
      'canvas2d/EventHitRegion.h',
      'canvas2d/HitRegion.cpp',
      'canvas2d/HitRegion.h',
      'canvas2d/MouseEventHitRegion.cpp',
      'canvas2d/MouseEventHitRegion.h',
      'canvas2d/Path2D.h',
      'canvas2d/TouchHitRegion.cpp',
      'canvas2d/TouchHitRegion.h',
    """

    str_compositorworker = """
      'compositorworker/CompositorWorker.cpp',
      'compositorworker/CompositorWorker.h',
      'compositorworker/CompositorWorkerGlobalScope.cpp',
      'compositorworker/CompositorWorkerGlobalScope.h',
      'compositorworker/CompositorWorkerManager.cpp',
      'compositorworker/CompositorWorkerManager.h',
      'compositorworker/CompositorWorkerMessagingProxy.cpp',
      'compositorworker/CompositorWorkerMessagingProxy.h',
      'compositorworker/CompositorWorkerThread.cpp',
      'compositorworker/CompositorWorkerThread.h',
    """

    str_credentialmanager = """
      'credentialmanager/Credential.cpp',
      'credentialmanager/Credential.h',
      'credentialmanager/CredentialManagerClient.cpp',
      'credentialmanager/CredentialManagerClient.h',
      'credentialmanager/CredentialsContainer.cpp',
      'credentialmanager/CredentialsContainer.h',
      'credentialmanager/FederatedCredential.cpp',
      'credentialmanager/FederatedCredential.h',
      'credentialmanager/PasswordCredential.cpp',
      'credentialmanager/PasswordCredential.h',
      'credentialmanager/NavigatorCredentials.cpp',
      'credentialmanager/NavigatorCredentials.h',
    """

    str_crypto = """
      'crypto/Crypto.cpp',
      'crypto/Crypto.h',
      'crypto/CryptoHistograms.cpp',
      'crypto/CryptoHistograms.h',
      'crypto/CryptoKey.cpp',
      'crypto/CryptoKey.h',
      'crypto/CryptoResultImpl.cpp',
      'crypto/CryptoResultImpl.h',
      'crypto/DOMWindowCrypto.cpp',
      'crypto/DOMWindowCrypto.h',
      'crypto/NormalizeAlgorithm.cpp',
      'crypto/NormalizeAlgorithm.h',
      'crypto/SubtleCrypto.cpp',
      'crypto/SubtleCrypto.h',
      'crypto/WorkerGlobalScopeCrypto.cpp',
      'crypto/WorkerGlobalScopeCrypto.h',
    """

    str_device_light = """
      'device_light/DeviceLightController.cpp',
      'device_light/DeviceLightController.h',
      'device_light/DeviceLightDispatcher.cpp',
      'device_light/DeviceLightDispatcher.h',
      'device_light/DeviceLightEvent.cpp',
      'device_light/DeviceLightEvent.h',
    """

    str_device_orientation = """
      'device_orientation/DeviceAcceleration.cpp',
      'device_orientation/DeviceAcceleration.h',
      'device_orientation/DeviceMotionController.cpp',
      'device_orientation/DeviceMotionController.h',
      'device_orientation/DeviceMotionData.cpp',
      'device_orientation/DeviceMotionData.h',
      'device_orientation/DeviceMotionDispatcher.cpp',
      'device_orientation/DeviceMotionDispatcher.h',
      'device_orientation/DeviceMotionEvent.cpp',
      'device_orientation/DeviceMotionEvent.h',
      'device_orientation/DeviceOrientationController.cpp',
      'device_orientation/DeviceOrientationController.h',
      'device_orientation/DeviceOrientationData.cpp',
      'device_orientation/DeviceOrientationData.h',
      'device_orientation/DeviceOrientationDispatcher.cpp',
      'device_orientation/DeviceOrientationDispatcher.h',
      'device_orientation/DeviceOrientationEvent.cpp',
      'device_orientation/DeviceOrientationEvent.h',
      'device_orientation/DeviceOrientationInspectorAgent.cpp',
      'device_orientation/DeviceOrientationInspectorAgent.h',
      'device_orientation/DeviceRotationRate.cpp',
      'device_orientation/DeviceRotationRate.h',
    """

    str_donottrack = """
      'donottrack/NavigatorDoNotTrack.cpp',
      'donottrack/NavigatorDoNotTrack.h',
    """

    str_encoding = """
      'encoding/Encoding.cpp',
      'encoding/Encoding.h',
      'encoding/TextDecoder.cpp',
      'encoding/TextDecoder.h',
      'encoding/TextEncoder.cpp',
      'encoding/TextEncoder.h',
    """

    str_encryptedmedia = """
      'encryptedmedia/ContentDecryptionModuleResultPromise.cpp',
      'encryptedmedia/ContentDecryptionModuleResultPromise.h',
      'encryptedmedia/EncryptedMediaUtils.cpp',
      'encryptedmedia/EncryptedMediaUtils.h',
      'encryptedmedia/HTMLMediaElementEncryptedMedia.cpp',
      'encryptedmedia/HTMLMediaElementEncryptedMedia.h',
      'encryptedmedia/MediaEncryptedEvent.cpp',
      'encryptedmedia/MediaEncryptedEvent.h',
      'encryptedmedia/MediaKeyMessageEvent.cpp',
      'encryptedmedia/MediaKeyMessageEvent.h',
      'encryptedmedia/MediaKeySession.cpp',
      'encryptedmedia/MediaKeySession.h',
      'encryptedmedia/MediaKeyStatusMap.cpp',
      'encryptedmedia/MediaKeyStatusMap.h',
      'encryptedmedia/MediaKeySystemAccess.cpp',
      'encryptedmedia/MediaKeySystemAccess.h',
      'encryptedmedia/MediaKeys.cpp',
      'encryptedmedia/MediaKeys.h',
      'encryptedmedia/MediaKeysClient.h',
      'encryptedmedia/MediaKeysController.cpp',
      'encryptedmedia/MediaKeysController.h',
      'encryptedmedia/NavigatorRequestMediaKeySystemAccess.cpp',
      'encryptedmedia/NavigatorRequestMediaKeySystemAccess.h',
      'encryptedmedia/SimpleContentDecryptionModuleResultPromise.cpp',
      'encryptedmedia/SimpleContentDecryptionModuleResultPromise.h',
    """

    str_fetch = """
      'fetch/CompositeDataConsumerHandle.cpp',
      'fetch/CompositeDataConsumerHandle.h',
      'fetch/DataConsumerHandleUtil.cpp',
      'fetch/DataConsumerHandleUtil.h',
      'fetch/Body.cpp',
      'fetch/Body.h',
      'fetch/BodyStreamBuffer.cpp',
      'fetch/BodyStreamBuffer.h',
      'fetch/CrossThreadHolder.h',
      'fetch/DataConsumerTee.cpp',
      'fetch/DataConsumerTee.h',
      'fetch/FetchBlobDataConsumerHandle.cpp',
      'fetch/FetchBlobDataConsumerHandle.h',
      'fetch/FetchDataConsumerHandle.h',
      'fetch/FetchDataLoader.cpp',
      'fetch/FetchDataLoader.h',
      'fetch/FetchFormDataConsumerHandle.cpp',
      'fetch/FetchFormDataConsumerHandle.h',
      'fetch/FetchHeaderList.cpp',
      'fetch/FetchHeaderList.h',
      'fetch/FetchManager.cpp',
      'fetch/FetchManager.h',
      'fetch/FetchRequestData.cpp',
      'fetch/FetchRequestData.h',
      'fetch/FetchResponseData.cpp',
      'fetch/FetchResponseData.h',
      'fetch/GlobalFetch.cpp',
      'fetch/GlobalFetch.h',
      'fetch/Headers.cpp',
      'fetch/Headers.h',
      'fetch/Request.cpp',
      'fetch/Request.h',
      'fetch/RequestInit.cpp',
      'fetch/RequestInit.h',
      'fetch/Response.cpp',
      'fetch/Response.h',
      'fetch/ResponseInit.h',
    """

    str_filesystem = """
      'filesystem/DOMFilePath.cpp',
      'filesystem/DOMFilePath.h',
      'filesystem/DOMFileSystem.cpp',
      'filesystem/DOMFileSystem.h',
      'filesystem/DOMFileSystemBase.cpp',
      'filesystem/DOMFileSystemBase.h',
      'filesystem/DOMFileSystemSync.cpp',
      'filesystem/DOMFileSystemSync.h',
      'filesystem/DOMWindowFileSystem.cpp',
      'filesystem/DOMWindowFileSystem.h',
      'filesystem/DataTransferItemFileSystem.cpp',
      'filesystem/DataTransferItemFileSystem.h',
      'filesystem/DevToolsHostFileSystem.cpp',
      'filesystem/DevToolsHostFileSystem.h',
      'filesystem/DirectoryEntry.cpp',
      'filesystem/DirectoryEntry.h',
      'filesystem/DirectoryEntrySync.cpp',
      'filesystem/DirectoryEntrySync.h',
      'filesystem/DirectoryReader.cpp',
      'filesystem/DirectoryReader.h',
      'filesystem/DirectoryReaderBase.h',
      'filesystem/DirectoryReaderSync.cpp',
      'filesystem/DirectoryReaderSync.h',
      'filesystem/DraggedIsolatedFileSystemImpl.cpp',
      'filesystem/DraggedIsolatedFileSystemImpl.h',
      'filesystem/EntriesCallback.h',
      'filesystem/Entry.cpp',
      'filesystem/Entry.h',
      'filesystem/EntryBase.cpp',
      'filesystem/EntryBase.h',
      'filesystem/EntryCallback.h',
      'filesystem/EntrySync.cpp',
      'filesystem/EntrySync.h',
      'filesystem/ErrorCallback.h',
      'filesystem/FileCallback.h',
      'filesystem/FileEntry.cpp',
      'filesystem/FileEntry.h',
      'filesystem/FileEntrySync.cpp',
      'filesystem/FileEntrySync.h',
      'filesystem/FileSystemCallback.h',
      'filesystem/FileSystemCallbacks.cpp',
      'filesystem/FileSystemCallbacks.h',
      'filesystem/FileSystemClient.h',
      'filesystem/FileWriter.cpp',
      'filesystem/FileWriter.h',
      'filesystem/FileWriterBase.cpp',
      'filesystem/FileWriterBase.h',
      'filesystem/FileWriterBaseCallback.h',
      'filesystem/FileWriterCallback.h',
      'filesystem/FileWriterSync.cpp',
      'filesystem/FileWriterSync.h',
      'filesystem/HTMLInputElementFileSystem.cpp',
      'filesystem/HTMLInputElementFileSystem.h',
      'filesystem/InspectorFileSystemAgent.cpp',
      'filesystem/InspectorFileSystemAgent.h',
      'filesystem/LocalFileSystem.cpp',
      'filesystem/LocalFileSystem.h',
      'filesystem/Metadata.h',
      'filesystem/MetadataCallback.h',
      'filesystem/SyncCallbackHelper.h',
      'filesystem/WorkerGlobalScopeFileSystem.cpp',
      'filesystem/WorkerGlobalScopeFileSystem.h',
    """

    str_gamepad = """
      'gamepad/Gamepad.cpp',
      'gamepad/Gamepad.h',
      'gamepad/GamepadButton.cpp',
      'gamepad/GamepadButton.h',
      'gamepad/GamepadDispatcher.cpp',
      'gamepad/GamepadDispatcher.h',
      'gamepad/GamepadEvent.cpp',
      'gamepad/GamepadEvent.h',
      'gamepad/GamepadList.cpp',
      'gamepad/GamepadList.h',
      'gamepad/NavigatorGamepad.cpp',
      'gamepad/NavigatorGamepad.h',
    """

    str_geo_features = """
      'geofencing/CircularGeofencingRegion.cpp',
      'geofencing/CircularGeofencingRegion.h',
      'geofencing/Geofencing.cpp',
      'geofencing/Geofencing.h',
      'geofencing/GeofencingError.cpp',
      'geofencing/GeofencingError.h',
      'geofencing/GeofencingEvent.cpp',
      'geofencing/GeofencingEvent.h',
      'geofencing/GeofencingRegion.h',
      'geofencing/ServiceWorkerGlobalScopeGeofencing.h',
      'geofencing/ServiceWorkerRegistrationGeofencing.cpp',
      'geofencing/ServiceWorkerRegistrationGeofencing.h',
      'geofencing/WorkerNavigatorGeofencing.cpp',
      'geofencing/WorkerNavigatorGeofencing.h',
      'geolocation/Coordinates.cpp',
      'geolocation/Geolocation.cpp',
      'geolocation/GeolocationController.cpp',
      'geolocation/GeoNotifier.cpp',
      'geolocation/GeoNotifier.h',
      'geolocation/GeolocationWatchers.cpp',
      'geolocation/GeolocationWatchers.h',
      'geolocation/NavigatorGeolocation.cpp',
      'geolocation/NavigatorGeolocation.h',
    """

    str_imagebitmap = """
      'imagebitmap/WindowImageBitmapFactories.cpp',
      'imagebitmap/WindowImageBitmapFactories.h',
    """

    str_indexeddb = """
      'indexeddb/DOMWindowIndexedDatabase.cpp',
      'indexeddb/DOMWindowIndexedDatabase.h',
      'indexeddb/IDBAny.cpp',
      'indexeddb/IDBAny.h',
      'indexeddb/IDBCursor.cpp',
      'indexeddb/IDBCursor.h',
      'indexeddb/IDBCursorWithValue.cpp',
      'indexeddb/IDBCursorWithValue.h',
      'indexeddb/IDBDatabase.cpp',
      'indexeddb/IDBDatabase.h',
      'indexeddb/IDBDatabaseCallbacks.cpp',
      'indexeddb/IDBDatabaseCallbacks.h',
      'indexeddb/IDBEventDispatcher.cpp',
      'indexeddb/IDBEventDispatcher.h',
      'indexeddb/IDBFactory.cpp',
      'indexeddb/IDBFactory.h',
      'indexeddb/IDBHistograms.h',
      'indexeddb/IDBIndex.cpp',
      'indexeddb/IDBIndex.h',
      'indexeddb/IDBKey.cpp',
      'indexeddb/IDBKey.h',
      'indexeddb/IDBKeyPath.cpp',
      'indexeddb/IDBKeyPath.h',
      'indexeddb/IDBKeyRange.cpp',
      'indexeddb/IDBKeyRange.h',
      'indexeddb/IDBMetadata.cpp',
      'indexeddb/IDBMetadata.h',
      'indexeddb/IDBObjectStore.cpp',
      'indexeddb/IDBObjectStore.h',
      'indexeddb/IDBOpenDBRequest.cpp',
      'indexeddb/IDBOpenDBRequest.h',
      'indexeddb/IDBRequest.cpp',
      'indexeddb/IDBRequest.h',
      'indexeddb/IDBTracing.h',
      'indexeddb/IDBTransaction.cpp',
      'indexeddb/IDBTransaction.h',
      'indexeddb/IDBValue.cpp',
      'indexeddb/IDBValue.h',
      'indexeddb/IDBVersionChangeEvent.cpp',
      'indexeddb/IDBVersionChangeEvent.h',
      'indexeddb/IndexedDB.h',
      'indexeddb/IndexedDBClient.cpp',
      'indexeddb/IndexedDBClient.h',
      'indexeddb/InspectorIndexedDBAgent.cpp',
      'indexeddb/InspectorIndexedDBAgent.h',
      'indexeddb/WebIDBCallbacksImpl.cpp',
      'indexeddb/WebIDBCallbacksImpl.h',
      'indexeddb/WebIDBDatabaseCallbacksImpl.cpp',
      'indexeddb/WebIDBDatabaseCallbacksImpl.h',
      'indexeddb/WorkerGlobalScopeIndexedDatabase.cpp',
      'indexeddb/WorkerGlobalScopeIndexedDatabase.h',
    """

    str_mediasession = """
      'mediasession/HTMLMediaElementMediaSession.cpp',
      'mediasession/HTMLMediaElementMediaSession.h',
      'mediasession/MediaSession.cpp',
      'mediasession/MediaSession.h',
    """

    str_mediasource = """
      'mediasource/HTMLVideoElementMediaSource.cpp',
      'mediasource/HTMLVideoElementMediaSource.h',
      'mediasource/MediaSource.cpp',
      'mediasource/MediaSource.h',
      'mediasource/MediaSourceRegistry.cpp',
      'mediasource/MediaSourceRegistry.h',
      'mediasource/SourceBuffer.cpp',
      'mediasource/SourceBuffer.h',
      'mediasource/SourceBufferList.cpp',
      'mediasource/SourceBufferList.h',
      'mediasource/TrackDefault.cpp',
      'mediasource/TrackDefault.h',
      'mediasource/TrackDefaultList.cpp',
      'mediasource/TrackDefaultList.h',
      'mediasource/URLMediaSource.cpp',
      'mediasource/URLMediaSource.h',
      'mediasource/VideoPlaybackQuality.cpp',
      'mediasource/VideoPlaybackQuality.h',
    """

    str_mediastream = """
      'mediastream/MediaConstraintsImpl.cpp',
      'mediastream/MediaConstraintsImpl.h',
      'mediastream/MediaDeviceInfo.cpp',
      'mediastream/MediaDeviceInfo.h',
      'mediastream/MediaDevices.cpp',
      'mediastream/MediaDevices.h',
      'mediastream/MediaDevicesRequest.cpp',
      'mediastream/MediaDevicesRequest.h',
      'mediastream/MediaStream.cpp',
      'mediastream/MediaStream.h',
      'mediastream/MediaStreamEvent.cpp',
      'mediastream/MediaStreamEvent.h',
      'mediastream/MediaStreamRegistry.cpp',
      'mediastream/MediaStreamRegistry.h',
      'mediastream/MediaStreamTrack.cpp',
      'mediastream/MediaStreamTrack.h',
      'mediastream/MediaStreamTrackEvent.cpp',
      'mediastream/MediaStreamTrackEvent.h',
      'mediastream/MediaStreamTrackSourcesCallback.h',
      'mediastream/MediaStreamTrackSourcesRequestImpl.cpp',
      'mediastream/MediaStreamTrackSourcesRequestImpl.h',
      'mediastream/NavigatorMediaStream.cpp',
      'mediastream/NavigatorMediaStream.h',
      'mediastream/NavigatorUserMedia.cpp',
      'mediastream/NavigatorUserMedia.h',
      'mediastream/NavigatorUserMediaError.cpp',
      'mediastream/NavigatorUserMediaError.h',
      'mediastream/NavigatorUserMediaErrorCallback.h',
      'mediastream/NavigatorUserMediaSuccessCallback.h',
      'mediastream/RTCDTMFSender.cpp',
      'mediastream/RTCDTMFSender.h',
      'mediastream/RTCDTMFToneChangeEvent.cpp',
      'mediastream/RTCDTMFToneChangeEvent.h',
      'mediastream/RTCDataChannel.cpp',
      'mediastream/RTCDataChannel.h',
      'mediastream/RTCDataChannelEvent.cpp',
      'mediastream/RTCDataChannelEvent.h',
      'mediastream/RTCErrorCallback.h',
      'mediastream/RTCIceCandidate.cpp',
      'mediastream/RTCIceCandidate.h',
      'mediastream/RTCIceCandidateEvent.cpp',
      'mediastream/RTCIceCandidateEvent.h',
      'mediastream/RTCPeerConnection.cpp',
      'mediastream/RTCPeerConnection.h',
      'mediastream/RTCSessionDescription.cpp',
      'mediastream/RTCSessionDescription.h',
      'mediastream/RTCSessionDescriptionCallback.h',
      'mediastream/RTCSessionDescriptionRequestImpl.cpp',
      'mediastream/RTCSessionDescriptionRequestImpl.h',
      'mediastream/RTCStatsReport.cpp',
      'mediastream/RTCStatsReport.h',
      'mediastream/RTCStatsRequestImpl.cpp',
      'mediastream/RTCStatsRequestImpl.h',
      'mediastream/RTCStatsResponse.cpp',
      'mediastream/RTCStatsResponse.h',
      'mediastream/RTCVoidRequestImpl.cpp',
      'mediastream/RTCVoidRequestImpl.h',
      'mediastream/SourceInfo.cpp',
      'mediastream/SourceInfo.h',
      'mediastream/URLMediaStream.cpp',
      'mediastream/URLMediaStream.h',
      'mediastream/UserMediaClient.h',
      'mediastream/UserMediaController.cpp',
      'mediastream/UserMediaController.h',
      'mediastream/UserMediaRequest.cpp',
      'mediastream/UserMediaRequest.h',
    """

    str_navigatorconnect = """
      'navigatorconnect/AcceptConnectionObserver.cpp',
      'navigatorconnect/AcceptConnectionObserver.h',
      'navigatorconnect/CrossOriginServiceWorkerClient.cpp',
      'navigatorconnect/CrossOriginServiceWorkerClient.h',
      'navigatorconnect/NavigatorServices.cpp',
      'navigatorconnect/NavigatorServices.h',
      'navigatorconnect/ServicePort.cpp',
      'navigatorconnect/ServicePort.h',
      'navigatorconnect/ServicePortCollection.cpp',
      'navigatorconnect/ServicePortCollection.h',
      'navigatorconnect/ServicePortConnectEvent.cpp',
      'navigatorconnect/ServicePortConnectEvent.h',
      'navigatorconnect/ServiceWorkerGlobalScopeNavigatorConnect.h',
      'navigatorconnect/WorkerNavigatorServices.cpp',
      'navigatorconnect/WorkerNavigatorServices.h',
      'navigatorcontentutils/NavigatorContentUtils.cpp',
      'navigatorcontentutils/NavigatorContentUtils.h',
      'navigatorcontentutils/NavigatorContentUtilsClient.h',
    """

    str_netinfo = """
      'netinfo/NavigatorNetworkInformation.cpp',
      'netinfo/NavigatorNetworkInformation.h',
      'netinfo/NetworkInformation.cpp',
      'netinfo/NetworkInformation.h',
      'netinfo/WorkerNavigatorNetworkInformation.cpp',
      'netinfo/WorkerNavigatorNetworkInformation.h',
    """

    str_nfc = """
      'nfc/NavigatorNFC.cpp',
      'nfc/NavigatorNFC.h',
      'nfc/NFC.cpp',
      'nfc/NFC.h',
    """

    str_notifications = """
      'notifications/Notification.cpp',
      'notifications/Notification.h',
      'notifications/NotificationData.cpp',
      'notifications/NotificationData.h',
      'notifications/NotificationEvent.cpp',
      'notifications/NotificationEvent.h',
      'notifications/NotificationPermissionCallback.h',
      'notifications/NotificationPermissionClient.cpp',
      'notifications/NotificationPermissionClient.h',
      'notifications/ServiceWorkerGlobalScopeNotifications.h',
      'notifications/ServiceWorkerRegistrationNotifications.cpp',
      'notifications/ServiceWorkerRegistrationNotifications.h',
    """

    str_permissions = """
      'permissions/NavigatorPermissions.cpp',
      'permissions/NavigatorPermissions.h',
      'permissions/PermissionController.cpp',
      'permissions/PermissionController.h',
      'permissions/Permissions.cpp',
      'permissions/Permissions.h',
      'permissions/PermissionCallback.cpp',
      'permissions/PermissionCallback.h',
      'permissions/PermissionsCallback.cpp',
      'permissions/PermissionsCallback.h',
      'permissions/PermissionStatus.cpp',
      'permissions/PermissionStatus.h',
      'permissions/WorkerNavigatorPermissions.cpp',
      'permissions/WorkerNavigatorPermissions.h',
    """

    str_plugins = """
      'plugins/DOMMimeType.cpp',
      'plugins/DOMMimeType.h',
      'plugins/DOMMimeTypeArray.cpp',
      'plugins/DOMMimeTypeArray.h',
      'plugins/DOMPlugin.cpp',
      'plugins/DOMPlugin.h',
      'plugins/DOMPluginArray.cpp',
      'plugins/DOMPluginArray.h',
      'plugins/NavigatorPlugins.cpp',
      'plugins/NavigatorPlugins.h',
      'plugins/PluginOcclusionSupport.cpp',
      'plugins/PluginOcclusionSupport.h',
    """

    str_presentation = """
      'presentation/NavigatorPresentation.cpp',
      'presentation/NavigatorPresentation.h',
      'presentation/Presentation.cpp',
      'presentation/Presentation.h',
      'presentation/PresentationAvailability.cpp',
      'presentation/PresentationAvailability.h',
      'presentation/PresentationAvailabilityCallbacks.cpp',
      'presentation/PresentationAvailabilityCallbacks.h',
      'presentation/PresentationController.cpp',
      'presentation/PresentationController.h',
      'presentation/PresentationError.cpp',
      'presentation/PresentationError.h',
      'presentation/PresentationRequest.cpp',
      'presentation/PresentationRequest.h',
      'presentation/PresentationSession.cpp',
      'presentation/PresentationSession.h',
      'presentation/PresentationSessionCallbacks.cpp',
      'presentation/PresentationSessionCallbacks.h',
      'presentation/PresentationSessionConnectEvent.cpp',
      'presentation/PresentationSessionConnectEvent.h',
    """

    str_push_messaging = """
      'push_messaging/PushController.cpp',
      'push_messaging/PushController.h',
      'push_messaging/PushError.cpp',
      'push_messaging/PushError.h',
      'push_messaging/PushEvent.cpp',
      'push_messaging/PushEvent.h',
      'push_messaging/PushManager.cpp',
      'push_messaging/PushManager.h',
      'push_messaging/PushMessageData.cpp',
      'push_messaging/PushMessageData.h',
      'push_messaging/PushPermissionStatusCallbacks.cpp',
      'push_messaging/PushPermissionStatusCallbacks.h',
      'push_messaging/PushSubscription.cpp',
      'push_messaging/PushSubscription.h',
      'push_messaging/PushSubscriptionCallbacks.cpp',
      'push_messaging/PushSubscriptionCallbacks.h',
      'push_messaging/ServiceWorkerGlobalScopePush.h',
      'push_messaging/ServiceWorkerRegistrationPush.cpp',
      'push_messaging/ServiceWorkerRegistrationPush.h',
    """

    str_quota = """
      'quota/DOMWindowQuota.cpp',
      'quota/DOMWindowQuota.h',
      'quota/DeprecatedStorageInfo.cpp',
      'quota/DeprecatedStorageInfo.h',
      'quota/DeprecatedStorageQuota.cpp',
      'quota/DeprecatedStorageQuota.h',
      'quota/DeprecatedStorageQuotaCallbacksImpl.cpp',
      'quota/DeprecatedStorageQuotaCallbacksImpl.h',
      'quota/NavigatorStorageQuota.cpp',
      'quota/NavigatorStorageQuota.h',
      'quota/StorageErrorCallback.cpp',
      'quota/StorageErrorCallback.h',
      'quota/StorageInfo.cpp',
      'quota/StorageInfo.h',
      'quota/StorageManager.cpp',
      'quota/StorageManager.h',
      'quota/StorageQuota.cpp',
      'quota/StorageQuota.h',
      'quota/StorageQuotaCallback.h',
      'quota/StorageQuotaCallbacksImpl.cpp',
      'quota/StorageQuotaCallbacksImpl.h',
      'quota/StorageQuotaClient.cpp',
      'quota/StorageQuotaClient.h',
      'quota/StorageUsageCallback.h',
      'quota/WorkerNavigatorStorageQuota.cpp',
      'quota/WorkerNavigatorStorageQuota.h',
    """

    str_screen_orientation = """
      'screen_orientation/LockOrientationCallback.cpp',
      'screen_orientation/LockOrientationCallback.h',
      'screen_orientation/ScreenScreenOrientation.cpp',
      'screen_orientation/ScreenScreenOrientation.h',
      'screen_orientation/ScreenOrientation.cpp',
      'screen_orientation/ScreenOrientation.h',
      'screen_orientation/ScreenOrientationController.cpp',
      'screen_orientation/ScreenOrientationController.h',
      'screen_orientation/ScreenOrientationDispatcher.cpp',
      'screen_orientation/ScreenOrientationDispatcher.h',
      'screen_orientation/ScreenOrientationInspectorAgent.cpp',
      'screen_orientation/ScreenOrientationInspectorAgent.h',
    """

    str_serviceworkers = """
      'serviceworkers/ExtendableEvent.cpp',
      'serviceworkers/ExtendableEvent.h',
      'serviceworkers/FetchEvent.cpp',
      'serviceworkers/FetchEvent.h',
      'serviceworkers/NavigatorServiceWorker.cpp',
      'serviceworkers/NavigatorServiceWorker.h',
      'serviceworkers/RespondWithObserver.cpp',
      'serviceworkers/RespondWithObserver.h',
      'serviceworkers/ServiceWorker.cpp',
      'serviceworkers/ServiceWorker.h',
      'serviceworkers/ServiceWorkerClient.cpp',
      'serviceworkers/ServiceWorkerClient.h',
      'serviceworkers/ServiceWorkerClients.cpp',
      'serviceworkers/ServiceWorkerClients.h',
      'serviceworkers/ServiceWorkerContainer.cpp',
      'serviceworkers/ServiceWorkerContainer.h',
      'serviceworkers/ServiceWorkerContainerClient.cpp',
      'serviceworkers/ServiceWorkerContainerClient.h',
      'serviceworkers/ServiceWorkerWindowClient.cpp',
      'serviceworkers/ServiceWorkerWindowClient.h',
      'serviceworkers/ServiceWorkerError.cpp',
      'serviceworkers/ServiceWorkerError.h',
      'serviceworkers/ServiceWorkerGlobalScope.cpp',
      'serviceworkers/ServiceWorkerGlobalScope.h',
      'serviceworkers/ServiceWorkerGlobalScopeClient.cpp',
      'serviceworkers/ServiceWorkerGlobalScopeClient.h',
      'serviceworkers/ServiceWorkerMessageEvent.cpp',
      'serviceworkers/ServiceWorkerMessageEvent.h',
      'serviceworkers/ServiceWorkerRegistration.cpp',
      'serviceworkers/ServiceWorkerRegistration.h',
      'serviceworkers/ServiceWorkerScriptCachedMetadataHandler.cpp',
      'serviceworkers/ServiceWorkerScriptCachedMetadataHandler.h',
      'serviceworkers/ServiceWorkerThread.cpp',
      'serviceworkers/ServiceWorkerThread.h',
      'serviceworkers/WaitUntilObserver.cpp',
    """

    str_speech = """
      'speech/DOMWindowSpeechSynthesis.cpp',
      'speech/DOMWindowSpeechSynthesis.h',
      'speech/SpeechGrammar.cpp',
      'speech/SpeechGrammar.h',
      'speech/SpeechGrammarList.cpp',
      'speech/SpeechGrammarList.h',
      'speech/SpeechRecognition.cpp',
      'speech/SpeechRecognition.h',
      'speech/SpeechRecognitionAlternative.cpp',
      'speech/SpeechRecognitionAlternative.h',
      'speech/SpeechRecognitionClient.h',
      'speech/SpeechRecognitionController.cpp',
      'speech/SpeechRecognitionController.h',
      'speech/SpeechRecognitionError.cpp',
      'speech/SpeechRecognitionError.h',
      'speech/SpeechRecognitionEvent.cpp',
      'speech/SpeechRecognitionEvent.h',
      'speech/SpeechRecognitionResult.cpp',
      'speech/SpeechRecognitionResult.h',
      'speech/SpeechRecognitionResultList.cpp',
      'speech/SpeechRecognitionResultList.h',
      'speech/SpeechSynthesis.cpp',
      'speech/SpeechSynthesis.h',
      'speech/SpeechSynthesisEvent.cpp',
      'speech/SpeechSynthesisEvent.h',
      'speech/SpeechSynthesisUtterance.cpp',
      'speech/SpeechSynthesisUtterance.h',
      'speech/SpeechSynthesisVoice.cpp',
      'speech/SpeechSynthesisVoice.h',
    """

    str_storage = """
      'storage/DOMWindowStorage.cpp',
      'storage/DOMWindowStorage.h',
      'storage/DOMWindowStorageController.cpp',
      'storage/DOMWindowStorageController.h',
      'storage/InspectorDOMStorageAgent.cpp',
      'storage/InspectorDOMStorageAgent.h',
      'storage/Storage.cpp',
      'storage/Storage.h',
      'storage/StorageArea.cpp',
      'storage/StorageArea.h',
      'storage/StorageEvent.cpp',
      'storage/StorageEvent.h',
      'storage/StorageNamespace.cpp',
      'storage/StorageNamespace.h',
      'storage/StorageNamespaceController.cpp',
      'storage/StorageNamespaceController.h',
    """

    str_vibration = """
      'vibration/NavigatorVibration.cpp',
      'vibration/NavigatorVibration.h',
    """

    str_vr = """
      'vr/HMDVRDevice.cpp',
      'vr/HMDVRDevice.h',
      'vr/NavigatorVRDevice.cpp',
      'vr/NavigatorVRDevice.h',
      'vr/PositionSensorVRDevice.cpp',
      'vr/PositionSensorVRDevice.h',
      'vr/VRController.cpp',
      'vr/VRController.h',
      'vr/VRDevice.cpp',
      'vr/VRDevice.h',
      'vr/VREyeParameters.cpp',
      'vr/VREyeParameters.h',
      'vr/VRFieldOfView.h',
      'vr/VRGetDevicesCallback.cpp',
      'vr/VRGetDevicesCallback.h',
      'vr/VRHardwareUnit.cpp',
      'vr/VRHardwareUnit.h',
      'vr/VRHardwareUnitCollection.cpp',
      'vr/VRHardwareUnitCollection.h',
      'vr/VRPositionState.cpp',
      'vr/VRPositionState.h',
    """

    str_wake_lock = """
      'wake_lock/ScreenWakeLock.cpp',
      'wake_lock/ScreenWakeLock.h',
    """

    str_webaudio = """
      'webaudio/AbstractAudioContext.cpp',
      'webaudio/AbstractAudioContext.h',
      'webaudio/AnalyserNode.cpp',
      'webaudio/AnalyserNode.h',
      'webaudio/AsyncAudioDecoder.cpp',
      'webaudio/AsyncAudioDecoder.h',
      'webaudio/AudioBasicInspectorNode.cpp',
      'webaudio/AudioBasicInspectorNode.h',
      'webaudio/AudioBasicProcessorHandler.cpp',
      'webaudio/AudioBasicProcessorHandler.h',
      'webaudio/AudioBuffer.cpp',
      'webaudio/AudioBuffer.h',
      'webaudio/AudioBufferCallback.h',
      'webaudio/AudioBufferSourceNode.cpp',
      'webaudio/AudioBufferSourceNode.h',
      'webaudio/AudioContext.cpp',
      'webaudio/AudioContext.h',
      'webaudio/AudioDestinationNode.cpp',
      'webaudio/AudioDestinationNode.h',
      'webaudio/AudioListener.cpp',
      'webaudio/AudioListener.h',
      'webaudio/AudioNode.cpp',
      'webaudio/AudioNode.h',
      'webaudio/AudioNodeInput.cpp',
      'webaudio/AudioNodeInput.h',
      'webaudio/AudioNodeOutput.cpp',
      'webaudio/AudioNodeOutput.h',
      'webaudio/AudioParam.cpp',
      'webaudio/AudioParam.h',
      'webaudio/AudioParamTimeline.cpp',
      'webaudio/AudioParamTimeline.h',
      'webaudio/AudioProcessingEvent.cpp',
      'webaudio/AudioProcessingEvent.h',
      'webaudio/AudioScheduledSourceNode.cpp',
      'webaudio/AudioScheduledSourceNode.h',
      'webaudio/AudioSourceNode.h',
      'webaudio/AudioSummingJunction.cpp',
      'webaudio/AudioSummingJunction.h',
      'webaudio/BiquadDSPKernel.cpp',
      'webaudio/BiquadDSPKernel.h',
      'webaudio/BiquadFilterNode.cpp',
      'webaudio/BiquadFilterNode.h',
      'webaudio/BiquadProcessor.cpp',
      'webaudio/BiquadProcessor.h',
      'webaudio/ChannelMergerNode.cpp',
      'webaudio/ChannelMergerNode.h',
      'webaudio/ChannelSplitterNode.cpp',
      'webaudio/ChannelSplitterNode.h',
      'webaudio/ConvolverNode.cpp',
      'webaudio/ConvolverNode.h',
      'webaudio/DefaultAudioDestinationNode.cpp',
      'webaudio/DefaultAudioDestinationNode.h',
      'webaudio/DeferredTaskHandler.cpp',
      'webaudio/DeferredTaskHandler.h',
      'webaudio/DelayDSPKernel.cpp',
      'webaudio/DelayDSPKernel.h',
      'webaudio/DelayNode.cpp',
      'webaudio/DelayNode.h',
      'webaudio/DelayProcessor.cpp',
      'webaudio/DelayProcessor.h',
      'webaudio/DynamicsCompressorNode.cpp',
      'webaudio/DynamicsCompressorNode.h',
      'webaudio/GainNode.cpp',
      'webaudio/GainNode.h',
      'webaudio/MediaElementAudioSourceNode.cpp',
      'webaudio/MediaElementAudioSourceNode.h',
      'webaudio/MediaStreamAudioDestinationNode.cpp',
      'webaudio/MediaStreamAudioDestinationNode.h',
      'webaudio/MediaStreamAudioSourceNode.cpp',
      'webaudio/MediaStreamAudioSourceNode.h',
      'webaudio/OfflineAudioCompletionEvent.cpp',
      'webaudio/OfflineAudioCompletionEvent.h',
      'webaudio/OfflineAudioContext.cpp',
      'webaudio/OfflineAudioContext.h',
      'webaudio/OfflineAudioDestinationNode.cpp',
      'webaudio/OfflineAudioDestinationNode.h',
      'webaudio/OscillatorNode.cpp',
      'webaudio/OscillatorNode.h',
      'webaudio/PannerNode.cpp',
      'webaudio/PannerNode.h',
      'webaudio/PeriodicWave.cpp',
      'webaudio/PeriodicWave.h',
      'webaudio/RealtimeAnalyser.cpp',
      'webaudio/RealtimeAnalyser.h',
      'webaudio/ScriptProcessorNode.cpp',
      'webaudio/ScriptProcessorNode.h',
      'webaudio/StereoPannerNode.cpp',
      'webaudio/StereoPannerNode.h',
      'webaudio/WaveShaperDSPKernel.cpp',
      'webaudio/WaveShaperDSPKernel.h',
      'webaudio/WaveShaperNode.cpp',
      'webaudio/WaveShaperNode.h',
      'webaudio/WaveShaperProcessor.cpp',
      'webaudio/WaveShaperProcessor.h',
    """

    str_webcl = """
      'webcl/DOMWindowWebCL.cpp',
      'webcl/DOMWindowWebCL.h',
      'webcl/WebCL.cpp',
      'webcl/WebCL.h',
      'webcl/WebCLBuffer.cpp',
      'webcl/WebCLBuffer.h',
      'webcl/WebCLCallback.h',
      'webcl/WebCLCommandQueue.cpp',
      'webcl/WebCLCommandQueue.h',
      'webcl/WebCLConfig.h',
      'webcl/WebCLContext.cpp',
      'webcl/WebCLContext.h',
      'webcl/WebCLDevice.cpp',
      'webcl/WebCLDevice.h',
      'webcl/WebCLEvent.cpp',
      'webcl/WebCLEvent.h',
      'webcl/WebCLExtension.cpp',
      'webcl/WebCLExtension.h',
      'webcl/WebCLHTMLUtil.cpp',
      'webcl/WebCLHTMLUtil.h',
      'webcl/WebCLImage.cpp',
      'webcl/WebCLImage.h',
      'webcl/WebCLInputChecker.cpp',
      'webcl/WebCLInputChecker.h',
      'webcl/WebCLKernel.cpp',
      'webcl/WebCLKernel.h',
      'webcl/WebCLKernelArgInfo.h',
      'webcl/WebCLKernelArgInfoProvider.cpp',
      'webcl/WebCLKernelArgInfoProvider.h',
      'webcl/WebCLMemoryObject.cpp',
      'webcl/WebCLMemoryObject.h',
      'webcl/WebCLMemoryUtil.cpp',
      'webcl/WebCLMemoryUtil.h',
      'webcl/WebCLObject.cpp',
      'webcl/WebCLObject.h',
      'webcl/WebCLOpenCL.cpp',
      'webcl/WebCLOpenCL.h',
      'webcl/WebCLPlatform.cpp',
      'webcl/WebCLPlatform.h',
      'webcl/WebCLProgram.cpp',
      'webcl/WebCLProgram.h',
      'webcl/WebCLSampler.cpp',
      'webcl/WebCLSampler.h',
      'webcl/WebCLUserEvent.cpp',
      'webcl/WebCLUserEvent.h',
    """

    str_webdatabase = """
      'webdatabase/ChangeVersionData.h',
      'webdatabase/ChangeVersionWrapper.cpp',
      'webdatabase/ChangeVersionWrapper.h',
      'webdatabase/DOMWindowWebDatabase.cpp',
      'webdatabase/DOMWindowWebDatabase.h',
      'webdatabase/Database.cpp',
      'webdatabase/Database.h',
      'webdatabase/DatabaseAuthorizer.cpp',
      'webdatabase/DatabaseAuthorizer.h',
      'webdatabase/DatabaseBasicTypes.h',
      'webdatabase/DatabaseCallback.h',
      'webdatabase/DatabaseClient.cpp',
      'webdatabase/DatabaseClient.h',
      'webdatabase/DatabaseContext.cpp',
      'webdatabase/DatabaseContext.h',
      'webdatabase/DatabaseError.h',
      'webdatabase/DatabaseManager.cpp',
      'webdatabase/DatabaseManager.h',
      'webdatabase/DatabaseTask.cpp',
      'webdatabase/DatabaseTask.h',
      'webdatabase/DatabaseThread.cpp',
      'webdatabase/DatabaseThread.h',
      'webdatabase/DatabaseTracker.cpp',
      'webdatabase/DatabaseTracker.h',
      'webdatabase/InspectorDatabaseAgent.cpp',
      'webdatabase/InspectorDatabaseAgent.h',
      'webdatabase/InspectorDatabaseResource.cpp',
      'webdatabase/InspectorDatabaseResource.h',
      'webdatabase/QuotaTracker.cpp',
      'webdatabase/QuotaTracker.h',
      'webdatabase/SQLError.cpp',
      'webdatabase/SQLError.h',
      'webdatabase/SQLResultSet.cpp',
      'webdatabase/SQLResultSetRowList.cpp',
      'webdatabase/SQLStatement.cpp',
      'webdatabase/SQLStatement.h',
      'webdatabase/SQLStatementBackend.cpp',
      'webdatabase/SQLStatementBackend.h',
      'webdatabase/SQLTransaction.cpp',
      'webdatabase/SQLTransaction.h',
      'webdatabase/SQLTransactionBackend.cpp',
      'webdatabase/SQLTransactionBackend.h',
      'webdatabase/SQLTransactionClient.cpp',
      'webdatabase/SQLTransactionClient.h',
      'webdatabase/SQLTransactionCoordinator.cpp',
      'webdatabase/SQLTransactionCoordinator.h',
      'webdatabase/SQLTransactionState.h',
      'webdatabase/SQLTransactionStateMachine.cpp',
      'webdatabase/SQLTransactionStateMachine.h',
      'webdatabase/sqlite/SQLValue.cpp',
      'webdatabase/sqlite/SQLiteAuthorizer.cpp',
      'webdatabase/sqlite/SQLiteDatabase.cpp',
      'webdatabase/sqlite/SQLiteDatabase.h',
      'webdatabase/sqlite/SQLiteFileSystem.cpp',
      'webdatabase/sqlite/SQLiteFileSystem.h',
      'webdatabase/sqlite/SQLiteFileSystemPosix.cpp',
      'webdatabase/sqlite/SQLiteFileSystemWin.cpp',
      'webdatabase/sqlite/SQLiteStatement.cpp',
      'webdatabase/sqlite/SQLiteStatement.h',
      'webdatabase/sqlite/SQLiteTransaction.cpp',
      'webdatabase/sqlite/SQLiteTransaction.h',
    """

    str_webgl = """
      'webgl/ANGLEInstancedArrays.cpp',
      'webgl/ANGLEInstancedArrays.h',
      'webgl/CHROMIUMSubscribeUniform.cpp',
      'webgl/CHROMIUMSubscribeUniform.h',
      'webgl/CHROMIUMValuebuffer.cpp',
      'webgl/CHROMIUMValuebuffer.h',
      'webgl/EXTBlendMinMax.cpp',
      'webgl/EXTBlendMinMax.h',
      'webgl/EXTFragDepth.cpp',
      'webgl/EXTFragDepth.h',
      'webgl/EXTShaderTextureLOD.cpp',
      'webgl/EXTShaderTextureLOD.h',
      'webgl/EXTTextureFilterAnisotropic.cpp',
      'webgl/EXTTextureFilterAnisotropic.h',
      'webgl/EXTsRGB.cpp',
      'webgl/EXTsRGB.h',
      'webgl/OESElementIndexUint.cpp',
      'webgl/OESElementIndexUint.h',
      'webgl/OESStandardDerivatives.cpp',
      'webgl/OESStandardDerivatives.h',
      'webgl/OESTextureFloat.cpp',
      'webgl/OESTextureFloat.h',
      'webgl/OESTextureFloatLinear.cpp',
      'webgl/OESTextureFloatLinear.h',
      'webgl/OESTextureHalfFloat.cpp',
      'webgl/OESTextureHalfFloat.h',
      'webgl/OESTextureHalfFloatLinear.cpp',
      'webgl/OESTextureHalfFloatLinear.h',
      'webgl/OESVertexArrayObject.cpp',
      'webgl/OESVertexArrayObject.h',
      'webgl/WebGL2RenderingContext.cpp',
      'webgl/WebGL2RenderingContext.h',
      'webgl/WebGL2RenderingContextBase.cpp',
      'webgl/WebGL2RenderingContextBase.h',
      'webgl/WebGLActiveInfo.h',
      'webgl/WebGLBuffer.cpp',
      'webgl/WebGLBuffer.h',
      'webgl/WebGLCompressedTextureATC.cpp',
      'webgl/WebGLCompressedTextureATC.h',
      'webgl/WebGLCompressedTextureETC1.cpp',
      'webgl/WebGLCompressedTextureETC1.h',
      'webgl/WebGLCompressedTexturePVRTC.cpp',
      'webgl/WebGLCompressedTexturePVRTC.h',
      'webgl/WebGLCompressedTextureS3TC.cpp',
      'webgl/WebGLCompressedTextureS3TC.h',
      'webgl/WebGLContextAttributeHelpers.cpp',
      'webgl/WebGLContextAttributeHelpers.h',
      'webgl/WebGLContextEvent.cpp',
      'webgl/WebGLContextEvent.h',
      'webgl/WebGLContextGroup.cpp',
      'webgl/WebGLContextGroup.h',
      'webgl/WebGLContextObject.cpp',
      'webgl/WebGLContextObject.h',
      'webgl/WebGLDebugRendererInfo.cpp',
      'webgl/WebGLDebugRendererInfo.h',
      'webgl/WebGLDebugShaders.cpp',
      'webgl/WebGLDebugShaders.h',
      'webgl/WebGLDepthTexture.cpp',
      'webgl/WebGLDepthTexture.h',
      'webgl/WebGLDrawBuffers.cpp',
      'webgl/WebGLDrawBuffers.h',
      'webgl/WebGLExtension.cpp',
      'webgl/WebGLExtension.h',
      'webgl/WebGLExtensionName.h',
      'webgl/WebGLFenceSync.cpp',
      'webgl/WebGLFenceSync.h',
      'webgl/WebGLFramebuffer.cpp',
      'webgl/WebGLFramebuffer.h',
      'webgl/WebGLLoseContext.cpp',
      'webgl/WebGLLoseContext.h',
      'webgl/WebGLObject.cpp',
      'webgl/WebGLObject.h',
      'webgl/WebGLProgram.cpp',
      'webgl/WebGLProgram.h',
      'webgl/WebGLQuery.cpp',
      'webgl/WebGLQuery.h',
      'webgl/WebGLRenderbuffer.cpp',
      'webgl/WebGLRenderbuffer.h',
      'webgl/WebGLRenderingContext.cpp',
      'webgl/WebGLRenderingContext.h',
      'webgl/WebGLRenderingContextBase.cpp',
      'webgl/WebGLRenderingContextBase.h',
      'webgl/WebGLSampler.cpp',
      'webgl/WebGLSampler.h',
      'webgl/WebGLShader.cpp',
      'webgl/WebGLShader.h',
      'webgl/WebGLShaderPrecisionFormat.cpp',
      'webgl/WebGLShaderPrecisionFormat.h',
      'webgl/WebGLSharedObject.cpp',
      'webgl/WebGLSharedObject.h',
      'webgl/WebGLSharedPlatform3DObject.cpp',
      'webgl/WebGLSharedPlatform3DObject.h',
      'webgl/WebGLSync.cpp',
      'webgl/WebGLSync.h',
      'webgl/WebGLTexture.cpp',
      'webgl/WebGLTexture.h',
      'webgl/WebGLTransformFeedback.cpp',
      'webgl/WebGLTransformFeedback.h',
      'webgl/WebGLUniformLocation.cpp',
      'webgl/WebGLUniformLocation.h',
      'webgl/WebGLVertexArrayObject.cpp',
      'webgl/WebGLVertexArrayObject.h',
      'webgl/WebGLVertexArrayObjectBase.cpp',
      'webgl/WebGLVertexArrayObjectBase.h',
      'webgl/WebGLVertexArrayObjectOES.cpp',
      'webgl/WebGLVertexArrayObjectOES.h',
    """

    str_webmidi = """
      'webmidi/MIDIAccess.cpp',
      'webmidi/MIDIAccess.h',
      'webmidi/MIDIAccessInitializer.cpp',
      'webmidi/MIDIAccessInitializer.h',
      'webmidi/MIDIAccessor.cpp',
      'webmidi/MIDIAccessor.h',
      'webmidi/MIDIAccessorClient.h',
      'webmidi/MIDIClient.h',
      'webmidi/MIDIConnectionEvent.cpp',
      'webmidi/MIDIConnectionEvent.h',
      'webmidi/MIDIController.cpp',
      'webmidi/MIDIController.h',
      'webmidi/MIDIInput.cpp',
      'webmidi/MIDIInput.h',
      'webmidi/MIDIInputMap.cpp',
      'webmidi/MIDIInputMap.h',
      'webmidi/MIDIMessageEvent.cpp',
      'webmidi/MIDIMessageEvent.h',
      'webmidi/MIDIPortMap.h',
      'webmidi/MIDIOutput.cpp',
      'webmidi/MIDIOutput.h',
      'webmidi/MIDIOutputMap.cpp',
      'webmidi/MIDIOutputMap.h',
      'webmidi/MIDIPort.cpp',
      'webmidi/MIDIPort.h',
      'webmidi/NavigatorWebMIDI.cpp',
      'webmidi/NavigatorWebMIDI.h',
    """

    str_websockets = """
      'websockets/CloseEvent.cpp',
      'websockets/CloseEvent.h',
      'websockets/DOMWebSocket.cpp',
      'websockets/DOMWebSocket.h',
      'websockets/DocumentWebSocketChannel.cpp',
      'websockets/DocumentWebSocketChannel.h',
      'websockets/InspectorWebSocketEvents.cpp',
      'websockets/InspectorWebSocketEvents.h',
      'websockets/WebSocketChannel.cpp',
      'websockets/WebSocketChannel.h',
      'websockets/WebSocketChannelClient.h',
      'websockets/WebSocketFrame.cpp',
      'websockets/WebSocketFrame.h',
      'websockets/WorkerWebSocketChannel.cpp',
      'websockets/WorkerWebSocketChannel.h',
    """

    str_webusb = """
      'webusb/NavigatorUSB.cpp',
      'webusb/NavigatorUSB.h',
      'webusb/USB.cpp',
      'webusb/USB.h',
      'webusb/USBAlternateInterface.cpp',
      'webusb/USBAlternateInterface.h',
      'webusb/USBConfiguration.cpp',
      'webusb/USBConfiguration.h',
      'webusb/USBConnectionEvent.cpp',
      'webusb/USBConnectionEvent.h',
      'webusb/USBController.cpp',
      'webusb/USBController.h',
      'webusb/USBDevice.cpp',
      'webusb/USBDevice.h',
      'webusb/USBEndpoint.cpp',
      'webusb/USBEndpoint.h',
      'webusb/USBError.cpp',
      'webusb/USBError.h',
      'webusb/USBInterface.cpp',
      'webusb/USBInterface.h',
    """

    str_tail = """
    ],
    """

    if not disable_accessibility:
        str = str + str_accessibility

    if not disable_app_banner:
        str = str + str_app_banner

    str = str + str_audio_output

    if not disable_background_sync:
        str = str + str_background_sync

    if not disable_battery:
        str = str + str_battery

    if not disable_beacon:
        str = str + str_beacon

    if not disable_bluetooth:
        str = str + str_bluetooth

    if not disable_cachestorage:
        str = str + str_cachestorage

    if not disable_canvas2d:
        str = str + str_canvas2d

    if not disable_compositorworker:
        str = str + str_compositorworker

    if not disable_credentialmanager:
        str = str + str_credentialmanager

    if not disable_crypto:
        str = str + str_crypto

    if not disable_device_light:
        str = str + str_device_light

    if not disable_device_orientation:
        str = str + str_device_orientation

    if not disable_donottrack:
        str = str + str_donottrack

    if not disable_encoding:
        str = str + str_encoding

    if not disable_encryptedmedia:
        str = str + str_encryptedmedia

    if not disable_fetch:
        str = str + str_fetch

    if not disable_filesystem:
        str = str + str_filesystem

    if not disable_gamepad:
        str = str + str_gamepad

    if not disable_geo_features:
        str = str + str_geo_features

    str = str + str_imagebitmap

    if not disable_indexeddb:
        str = str + str_indexeddb

    if not disable_mediasession:
        str = str + str_mediasession

    if not disable_mediasource:
        str = str + str_mediasource

    if not disable_mediastream:
        str = str + str_mediastream

    if not disable_navigatorconnect:
        str = str + str_navigatorconnect

    if not disable_netinfo:
        str = str + str_netinfo

    if not disable_nfc:
        str = str + str_nfc

    if not disable_notifications:
        str = str + str_notifications

    if not disable_permissions:
        str = str + str_permissions

    if not disable_plugins:
        str = str + str_plugins

    if not disable_presentation:
        str = str + str_presentation

    if not disable_push_messaging:
        str = str + str_push_messaging

    if not disable_quota:
        str = str + str_quota

    if not disable_screen_orientation:
        str = str + str_screen_orientation

    if not disable_serviceworkers:
        str = str + str_serviceworkers

    if not disable_speech:
        str = str + str_speech

    if not disable_storage:
        str = str + str_storage

    if not disable_vibration:
        str = str + str_vibration

    if not disable_vr:
        str = str + str_vr

    str = str + str_wake_lock

    if not disable_webaudio:
        str = str + str_webaudio

    if not disable_webcl:
        str = str + str_webcl

    if not disable_webdatabase:
        str = str + str_webdatabase

    if not disable_webgl:
        str = str + str_webgl

    if not disable_webmidi:
        str = str + str_webmidi

    if not disable_websockets:
        str = str + str_websockets

    if not disable_webusb:
        str = str + str_webusb

    return str + str_tail
####################################end modules_files

def modules_testing_dependency_idl_files():
    str = """
    # 'partial interface' or target (right side of) 'implements'
    'modules_testing_dependency_idl_files' : [
    """

    str_accessibility = """
      'accessibility/testing/InternalsAccessibility.idl',
    """

    str_geo_features = """
      'geolocation/testing/InternalsGeolocation.idl',
    """

    str_navigatorconnect = """
      'navigatorcontentutils/testing/InternalsNavigatorContentUtils.idl',
    """

    str_serviceworkers = """
      'serviceworkers/testing/InternalsServiceWorker.idl',
    """

    str_speech = """
      'speech/testing/InternalsSpeechSynthesis.idl',
    """

    str_vibration = """
      'vibration/testing/InternalsVibration.idl',
    """

    str_webaudio = """
      'webaudio/testing/InternalsWebAudio.idl',
    """

    str_tail = """
    ],
    """

    if not disable_accessibility:
        str = str + str_accessibility

    if not disable_geo_features:
        str = str + str_geo_features

    if not disable_navigatorconnect:
        str = str + str_navigatorconnect

    if not disable_serviceworkers:
        str = str + str_serviceworkers

    if not disable_speech:
        str = str + str_speech

    if not disable_vibration:
        str = str + str_vibration

    if not disable_webaudio:
        str = str + str_webaudio

    return str + str_tail
####################################end modules_testing_dependency_idl_files

def modules_testing_files():
    str = """
    'modules_testing_files': [
    """

    str_accessibility = """
      'accessibility/testing/InternalsAccessibility.cpp',
      'accessibility/testing/InternalsAccessibility.h',
    """

    str_geo_features = """
      'geolocation/testing/GeolocationClientMock.cpp',
      'geolocation/testing/GeolocationClientMock.h',
      'geolocation/testing/InternalsGeolocation.cpp',
      'geolocation/testing/InternalsGeolocation.h',
    """

    str_navigatorconnect = """
      'navigatorcontentutils/testing/InternalsNavigatorContentUtils.cpp',
      'navigatorcontentutils/testing/InternalsNavigatorContentUtils.h',
      'navigatorcontentutils/testing/NavigatorContentUtilsClientMock.cpp',
      'navigatorcontentutils/testing/NavigatorContentUtilsClientMock.h',
    """

    str_serviceworkers = """
      'serviceworkers/testing/InternalsServiceWorker.cpp',
      'serviceworkers/testing/InternalsServiceWorker.h',
    """

    str_speech = """
      'speech/testing/InternalsSpeechSynthesis.cpp',
      'speech/testing/InternalsSpeechSynthesis.h',
      'speech/testing/PlatformSpeechSynthesizerMock.cpp',
      'speech/testing/PlatformSpeechSynthesizerMock.h',
    """

    str_vibration = """
      'vibration/testing/InternalsVibration.cpp',
      'vibration/testing/InternalsVibration.h',
    """

    str_webaudio = """
      'webaudio/testing/InternalsWebAudio.h',
      'webaudio/testing/InternalsWebAudio.cpp',
    """

    str_tail = """
    ],
    """

    if not disable_accessibility:
        str = str + str_accessibility

    if not disable_geo_features:
        str = str + str_geo_features

    if not disable_navigatorconnect:
        str = str + str_navigatorconnect

    if not disable_serviceworkers:
        str = str + str_serviceworkers

    if not disable_speech:
        str = str + str_speech

    if not disable_vibration:
        str = str + str_vibration

    if not disable_webaudio:
        str = str + str_webaudio

    return str + str_tail
####################################end modules_testing_files

def modules_unittest_files():
    str = """
    'modules_unittest_files': [
    """

    str_accessibility = """
      'accessibility/AXObjectTest.cpp',
    """

    str_cachestorage = """
      'cachestorage/CacheTest.cpp',
    """

    str_canvas2d = """
      'canvas2d/CanvasRenderingContext2DAPITest.cpp',
      'canvas2d/CanvasRenderingContext2DTest.cpp',
    """

    str_compositorworker = """
      'compositorworker/CompositorWorkerManagerTest.cpp',
    """

    str_fetch = """
      'fetch/BodyStreamBufferTest.cpp',
      'fetch/CompositeDataConsumerHandleTest.cpp',
      'fetch/DataConsumerHandleTestUtil.cpp',
      'fetch/DataConsumerHandleTestUtil.h',
      'fetch/DataConsumerHandleUtilTest.cpp',
      'fetch/DataConsumerTeeTest.cpp',
      'fetch/FetchBlobDataConsumerHandleTest.cpp',
      'fetch/FetchDataLoaderTest.cpp',
      'fetch/FetchFormDataConsumerHandleTest.cpp',
      'fetch/FetchResponseDataTest.cpp',
      'fetch/RequestTest.cpp',
      'fetch/ResponseTest.cpp',
    """

    str_filesystem = """
      'filesystem/DOMFileSystemBaseTest.cpp',
    """

    str_imagebitmap = """
      'imagebitmap/ImageBitmapModuleTest.cpp',
    """

    str_indexeddb = """
      'indexeddb/IDBKeyPathTest.cpp',
      'indexeddb/IDBRequestTest.cpp',
      'indexeddb/IDBTransactionTest.cpp',
    """

    str_mediastream = """
      'mediastream/RTCDataChannelTest.cpp',
    """

    str_notifications = """
      'notifications/NotificationDataTest.cpp',
    """

    str_presentation = """
      'presentation/PresentationAvailabilityTest.cpp',
    """

    str_serviceworkers = """
      'serviceworkers/ServiceWorkerContainerTest.cpp',
    """

    str_webaudio = """
      'webaudio/AudioBasicProcessorHandlerTest.cpp',
      'webaudio/ConvolverNodeTest.cpp',
      'webaudio/DynamicsCompressorNodeTest.cpp',
      'webaudio/ScriptProcessorNodeTest.cpp',
      'webaudio/StereoPannerNodeTest.cpp',
    """

    str_websockets = """
      'websockets/DOMWebSocketTest.cpp',
      'websockets/DocumentWebSocketChannelTest.cpp',
    """

    str_tail = """
    ],
    """

    if not disable_accessibility:
        str = str + str_accessibility

    if not disable_cachestorage:
        str = str + str_cachestorage

    if not disable_canvas2d:
        str = str + str_canvas2d

    str = str + str_compositorworker;

    if not disable_fetch:
        str = str + str_fetch

    if not disable_filesystem:
        str = str + str_filesystem

    str = str + str_imagebitmap;

    if not disable_indexeddb:
        str = str + str_indexeddb

    if not disable_mediastream:
        str = str + str_mediastream;

    if not disable_notifications:
        str = str + str_notifications;

    if not disable_presentation:
        str = str + str_presentation;

    if not disable_serviceworkers:
        str = str + str_serviceworkers

    if not disable_webaudio:
        str = str + str_webaudio

    if not disable_websockets:
        str = str + str_websockets

    return str + str_tail
###################################end modules_unittest_files

def file_tail():
    str = """
  },
}
    """
    return str
####################################
# args : (accessibility, indexeddb, notifications, speech, webcl)
# The arguments are arranged in alphabetical order
def generate_modules_gypi(args):
    global disable_accessibility
    global disable_app_banner
    global disable_background_sync
    global disable_battery
    global disable_beacon
    global disable_bluetooth
    global disable_cachestorage
    global disable_canvas2d
    global disable_compositorworker
    global disable_credentialmanager
    global disable_crypto
    global disable_donottrack
    global disable_device_light
    global disable_device_orientation
    global disable_encoding
    global disable_encryptedmedia
    global disable_fetch
    global disable_filesystem
    global disable_gamepad
    global disable_geo_features
    global disable_indexeddb
    global disable_mediasession
    global disable_mediasource
    global disable_mediastream
    global disable_navigatorconnect
    global disable_netinfo
    global disable_nfc
    global disable_notifications
    global disable_performance
    global disable_permissions
    global disable_plugins
    global disable_presentation
    global disable_push_messaging
    global disable_quota
    global disable_screen_orientation
    global disable_serviceworkers
    global disable_speech
    global disable_storage
    global disable_vibration
    global disable_vr
    global disable_webaudio
    global disable_webcl
    global disable_webdatabase
    global disable_webgl
    global disable_webmidi
    global disable_websockets
    global disable_webusb

    disable_accessibility = 0
    disable_app_banner = 0
    disable_background_sync = 0
    disable_battery = 0
    disable_beacon = 0
    disable_bluetooth = 0
    disable_cachestorage = 0
    disable_canvas2d = 0
    disable_compositorworker = 0
    disable_credentialmanager = 0
    disable_crypto = 0
    disable_donottrack = 0
    disable_device_light = 0
    disable_device_orientation = 0
    disable_encoding = 0
    disable_encryptedmedia = 0
    disable_fetch = 0
    disable_filesystem = 0
    disable_gamepad = 0
    disable_geo_features = 0
    disable_indexeddb = 0
    disable_mediasession = 0
    disable_mediasource = 0
    disable_mediastream = 0
    disable_navigatorconnect = 0
    disable_netinfo = 0
    disable_nfc = 0
    disable_notifications = 0
    disable_performance = 0
    disable_permissions = 0
    disable_plugins = 0
    disable_presentation = 0
    disable_push_messaging = 0
    disable_quota = 0
    disable_screen_orientation = 0
    disable_serviceworkers = 0
    disable_speech = 0
    disable_storage = 0
    disable_vibration = 0
    disable_vr = 0
    disable_webaudio = 0
    disable_webcl = 0
    disable_webdatabase = 0
    disable_webgl = 0
    disable_webmidi = 0
    disable_websockets = 0
    disable_webusb = 0

    disable_accessibility = args[0]
    disable_bluetooth = args[1]
    disable_geo_features = args[2]
    disable_indexeddb = args[3]
    disable_mediastream = args[4]
    disable_notifications = args[5]
    disable_plugins = args[6]
    disable_speech = args[7]
    disable_webaudio = args[8]
    disable_webcl = args[9]
    disable_webdatabase = args[10]
    disable_webmidi = args[11]

    name = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'modules.gypi')

    if os.path.exists(name):
        os.remove(name)

    with open(name, 'a') as f:
        f.write(file_header())
        f.write(modules_idl_files())
        f.write(modules_dependency_idl_files())
        f.write(modules_event_idl_files())
        f.write(modules_dictionary_idl_files())
        f.write(generated_modules_files())
        f.write(generated_modules_dictionary_files())
        f.write(modules_files())
        f.write(modules_testing_dependency_idl_files())
        f.write(modules_testing_files())
        f.write(modules_unittest_files())
        f.write(file_tail())


