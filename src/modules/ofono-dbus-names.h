/*
 * ofono-dbus-names.h
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * ofono-dbus-names.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ofono-dbus-names.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once 

#define OFONO_SERVICE "org.ofono"
#define OFONO_SERVER_SERVICE "org.ofono.server"
#define OFONO_PREFIX_ERROR OFONO_SERVICE ".Error."
#define OFONO_MANAGER_PATH "/"

#define OFONO_MANAGER_IFACE "org.ofono.Manager"
#define OFONO_MODEM_IFACE "org.ofono.Modem"
#define OFONO_CALL_BARRING_IFACE "org.ofono.CallBarring"
#define OFONO_CALL_FORWARDING_IFACE "org.ofono.CallForwarding"
#define OFONO_CALL_METER_IFACE "org.ofono.CallMeter"
#define OFONO_CALL_SETTINGS_IFACE "org.ofono.CallSettings"
#define OFONO_CALL_VOLUME_IFACE "org.ofono.CallVolume"
#define OFONO_CELL_BROADCAST_IFACE "org.ofono.CellBroadcast"
#define OFONO_CONTEXT_IFACE "org.ofono.ConnectionContext"
#define OFONO_CONNMAN_IFACE "org.ofono.ConnectionManager"
#define OFONO_MESSAGE_MANAGER_IFACE "org.ofono.MessageManager"
#define OFONO_MESSAGE_IFACE "org.ofono.Message"
#define OFONO_SMART_MESSAGE_IFACE "org.ofono.SmartMessaging"
#define OFONO_MESSAGE_WAITING_IFACE "org.ofono.MessageWaiting"
#define OFONO_SUPPLEMENTARY_SERVICES_IFACE "org.ofono.SupplementaryServices"
#define OFONO_NETWORK_REGISTRATION_IFACE "org.ofono.NetworkRegistration"
#define OFONO_NETWORK_OPERATOR_IFACE "org.ofono.NetworkOperator"
#define OFONO_PHONEBOOK_IFACE "org.ofono.Phonebook"
#define OFONO_RADIO_SETTINGS_IFACE "org.ofono.RadioSettings"
#define OFONO_AUDIO_SETTINGS_IFACE "org.ofono.AudioSettings"
#define OFONO_TEXT_TELEPHONY_IFACE "org.ofono.TextTelephony"
#define OFONO_SIM_MANAGER_IFACE "org.ofono.SimManager"
#define OFONO_VOICECALL_IFACE "org.ofono.VoiceCall"
#define OFONO_VOICECALL_MANAGER_IFACE "org.ofono.VoiceCallManager"
#define OFONO_STK_IFACE "org.ofono.SimToolkit"
#define OFONO_SIM_APP_IFACE "org.ofono.SimToolkitAgent"
#define OFONO_LOCATION_REPORTING_IFACE "org.ofono..LocationReporting"
#define OFONO_GNSS_IFACE "org.ofono.AssistedSatelliteNavigation"
#define OFONO_GNSS_POSR_AGENT_IFACE "org.ofono.PositioningRequestAgent"
#define OFONO_HANDSFREE_IFACE  "org.ofono.Handsfree"
#define OFONO_SIRI_INTERFACE "org.ofono.Siri"
#define OFONO_NETMON_INTERFACE "org.ofono.NetworkMonitor"
#define OFONO_LTE_INTERFACE "org.ofono.LongTermEvolution"
#define OFONO_CDMA_VOICECALL_MANAGER_IFACE "org.ofono.cdma.VoiceCallManager"
#define OFONO_CDMA_MESSAGE_MANAGER_IFACE "org.ofono.cdma.MessageManager"
#define OFONO_CDMA_CONNECTION_MANAGER_IFACE "org.ofono.cdma.ConnectionManager"
#define OFONO_PUSH_NOTIFICATION_IFACE "org.ofono.PushNotification"
#define OFONO_CDMA_NETWORK_REGISTRATION_IFACE  "org.ofono.cdma.NetworkRegistration"
