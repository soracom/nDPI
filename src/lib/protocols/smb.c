/*
 * smb.c
 *
 * Copyright (C) 2016-21 - ntop.org
 *
 * This file is part of nDPI, an open source deep packet inspection
 * library based on the OpenDPI and PACE technology by ipoque GmbH
 *
 * nDPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * nDPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with nDPI.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "ndpi_protocol_ids.h"
#define NDPI_CURRENT_PROTO NDPI_PROTOCOL_SMBV23
#include "ndpi_api.h"


void ndpi_search_smb_tcp(struct ndpi_detection_module_struct *ndpi_struct, struct ndpi_flow_struct *flow)
{
  struct ndpi_packet_struct *packet = &ndpi_struct->packet;

  NDPI_LOG_DBG(ndpi_struct, "search SMB\n");

  /* Check connection over TCP */
  if(packet->tcp) {
    u_int16_t fourfourfive =  htons(445);
    
    if(((packet->tcp->dest == fourfourfive) || (packet->tcp->source == fourfourfive))
       && packet->payload_packet_len > (32 + 4 + 4)
       && ((uint32_t)packet->payload_packet_len - 4) == ntohl(get_u_int32_t(packet->payload, 0))
       ) {
      u_int8_t smbv1[] = { 0xff, 0x53, 0x4d, 0x42 };

      NDPI_LOG_INFO(ndpi_struct, "found SMB\n");

      if(memcmp(&packet->payload[4], smbv1, sizeof(smbv1)) == 0) {
	if(packet->payload[8] != 0x72) /* Skip Negotiate request */ {	  
	  ndpi_set_detected_protocol(ndpi_struct, flow, NDPI_PROTOCOL_SMBV1, NDPI_PROTOCOL_NETBIOS);
	  ndpi_set_risk(ndpi_struct, flow, NDPI_SMB_INSECURE_VERSION);
	}
      } else
	ndpi_set_detected_protocol(ndpi_struct, flow, NDPI_PROTOCOL_SMBV23, NDPI_PROTOCOL_NETBIOS);

      return;
    }
  }

  ndpi_exclude_protocol(ndpi_struct, flow, NDPI_PROTOCOL_SMBV1, __FILE__, __FUNCTION__, __LINE__);
  ndpi_exclude_protocol(ndpi_struct, flow, NDPI_PROTOCOL_SMBV23, __FILE__, __FUNCTION__, __LINE__);
}


void init_smb_dissector(struct ndpi_detection_module_struct *ndpi_struct, u_int32_t *id, NDPI_PROTOCOL_BITMASK *detection_bitmask)
{
  ndpi_set_bitmask_protocol_detection("SMB", ndpi_struct, detection_bitmask, *id,
				      NDPI_PROTOCOL_SMBV23,
				      ndpi_search_smb_tcp,
				      NDPI_SELECTION_BITMASK_PROTOCOL_V4_V6_TCP_WITH_PAYLOAD_WITHOUT_RETRANSMISSION,
				      SAVE_DETECTION_BITMASK_AS_UNKNOWN,
				      ADD_TO_DETECTION_BITMASK);

  *id += 1;
}
