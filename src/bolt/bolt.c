/*
 * Copyright FalkorDB Ltd. 2023 - present
 * Licensed under the Server Side Public License v1 (SSPLv1).
 */

#include "RG.h"
#include "bolt.h"
#include "string.h"
#include <arpa/inet.h>

void bolt_reply_null
(
	bolt_client_t *client
) {
	client->write_buffer[client->nwrite++] = 0xC0;
}

void bolt_reply_bool
(
	bolt_client_t *client,
	bool data
) {
	client->write_buffer[client->nwrite++] = data ? 0xC3 : 0xC2;
}

void bolt_reply_tiny_int
(
	bolt_client_t *client,
	uint8_t data
) {
	client->write_buffer[client->nwrite++] = data;
}

void bolt_reply_int8
(
	bolt_client_t *client,
	int8_t data
) {
	client->write_buffer[client->nwrite++] = 0xC8;
	client->write_buffer[client->nwrite++] = data;
}

void bolt_reply_int16
(
	bolt_client_t *client,
	int16_t data
) {
	client->write_buffer[client->nwrite++] = 0xC9;
	*(uint16_t *)(client->write_buffer + client->nwrite) = htons(data);
	client->nwrite += 2;
}

void bolt_reply_int32
(
	bolt_client_t *client,
	int32_t data
) {
	client->write_buffer[client->nwrite++] = 0xCA;
	*(uint32_t *)(client->write_buffer + client->nwrite) = htonl(data);
	client->nwrite += 4;
}

void bolt_reply_int64
(
	bolt_client_t *client,
	int64_t data
) {
	client->write_buffer[client->nwrite++] = 0xCB;
	*(uint64_t *)(client->write_buffer + client->nwrite) = htonll(data);
	client->nwrite += 8;
}

void bolt_reply_int
(
	bolt_client_t *client,
	int64_t data
) {
	if(data >= 0xF0 && data <= 0x7F) {
		bolt_reply_tiny_int(client, data);
	} else if(INT8_MIN <= data && data <= INT8_MAX) {
		bolt_reply_int8(client, data);
	} else if(INT16_MIN <= data && data <= INT16_MAX) {
		bolt_reply_int16(client, data);
	} else if(INT32_MIN <= data && data <= INT32_MAX) {
		bolt_reply_int32(client, data);
	} else {
		bolt_reply_int64(client, data);
	}
}

void bolt_reply_float
(
	bolt_client_t *client,
	double data
) {
	client->write_buffer[client->nwrite++] = 0xC1;
	char *buf = (char *)&data;
	for (int i = 0; i < sizeof(double); i++) {
	  client->write_buffer[client->nwrite++] = buf[sizeof(double) - i - 1];
	}
}

void bolt_reply_string
(
	bolt_client_t *client,
	const char *data
) {
	uint32_t size = strlen(data);
	if (size < 0x10) {
		client->write_buffer[client->nwrite++] = 0x80 + size;
		memcpy(client->write_buffer + client->nwrite, data, size);
		client->nwrite += size;
	} else if (size < 0x100) {
		client->write_buffer[client->nwrite++] = 0xD0;
		client->write_buffer[client->nwrite++] = size;
		memcpy(client->write_buffer + client->nwrite, data, size);
		client->nwrite += size;
	} else if (size < 0x10000) {
		client->write_buffer[client->nwrite++] = 0xD1;
		*(uint16_t *)(client->write_buffer + client->nwrite) = htons(size);
		client->nwrite += 2;
		memcpy(client->write_buffer + client->nwrite, data, size);
		client->nwrite += size;
	} else {
		client->write_buffer[client->nwrite++] = 0xD2;
		*(uint32_t *)(client->write_buffer + client->nwrite) = htonl(size);
		client->nwrite += 4;
		memcpy(client->write_buffer + client->nwrite, data, size);
		client->nwrite += size;
	}
}

void bolt_reply_list
(
	bolt_client_t *client,
	uint32_t size
) {
	if (size < 0x10) {
		client->write_buffer[client->nwrite++] = 0x90 + size;
	} else if (size < 0x100) {
		client->write_buffer[client->nwrite++] = 0xD4;
		client->write_buffer[client->nwrite++] = size;
	} else if (size < 0x10000) {
		client->write_buffer[client->nwrite++] = 0xD5;
		*(uint16_t *)(client->write_buffer + client->nwrite) = htons(size);
		client->nwrite += 2;
	} else {
		client->write_buffer[client->nwrite++] = 0xD6;
		*(uint32_t *)(client->write_buffer + client->nwrite) = htons(size);
		client->nwrite += 4;
	}
}

void bolt_reply_map
(
	bolt_client_t *client,
	uint32_t size
) {
	if (size < 0x10) {
		client->write_buffer[client->nwrite++] = 0xA0 + size;
	} else if (size < 0x100) {
		client->write_buffer[client->nwrite++] = 0xD8;
		client->write_buffer[client->nwrite++] = size;
	} else if (size < 0x10000) {
		client->write_buffer[client->nwrite++] = 0xD9;
		*(uint16_t *)(client->write_buffer + client->nwrite) = htons(size);
		client->nwrite += 2;
	} else {
		client->write_buffer[client->nwrite++] = 0xDA;
		*(uint32_t *)(client->write_buffer + client->nwrite) = htonl(size);
		client->nwrite += 4;
	}
}

void bolt_reply_structure
(
	bolt_client_t *client,
	bolt_structure_type type,
	uint32_t size
) {
	client->write_buffer[client->nwrite++] = 0xB0 + size;
	client->write_buffer[client->nwrite++] = type;
}

char *bolt_value_read
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
		case 0x8E:
		case 0x8F:
			return data + 1 + marker - 0x80;
		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:
		case 0x9E:
		case 0x9F:
			data = data + 1;
			for (uint32_t i = 0; i < marker - 0x90; i++) {
				data = bolt_value_read(data);
			}
			return data;
		case 0xA0:
		case 0xA1:
		case 0xA2:
		case 0xA3:
		case 0xA4:
		case 0xA5:
		case 0xA6:
		case 0xA7:
		case 0xA8:
		case 0xA9:
		case 0xAA:
		case 0xAB:
		case 0xAC:
		case 0xAD:
		case 0xAE:
		case 0xAF:
			data = data + 1;
			for (uint32_t i = 0; i < marker - 0xA0; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			return data;
		case 0xB0:
		case 0xB1:
		case 0xB2:
		case 0xB3:
		case 0xB4:
		case 0xB5:
		case 0xB6:
		case 0xB7:
		case 0xB8:
		case 0xB9:
		case 0xBA:
		case 0xBB:
		case 0xBC:
		case 0xBD:
		case 0xBE:
		case 0xBF:
			data = data + 2;
			for (uint32_t i = 0; i < marker - 0xB0; i++) {
				data = bolt_value_read(data);
			}
			return data;
		case 0xC0:
			return data + 1;
		case 0xC1:
			return data + 9;
		case 0xC2:
			return data + 1;
		case 0xC3:
			return data + 1;
		case 0xC8:
			return data + 2;
		case 0xC9:
			return data + 3;
		case 0xCA:
			return data + 5;
		case 0xCB:
			return data + 9;
		case 0xCC:
			return data + 2 + *(uint8_t *)(data + 1);
		case 0xCD:
			return data + 3 + *(uint16_t *)(data + 1);
		case 0xCE:
			return data + 5 + *(uint32_t *)(data + 1);
		case 0xD0:
			return data + 2 + *(uint8_t *)(data + 1);
		case 0xD1:
			return data + 3 + ntohs(*(uint16_t *)(data + 1));
		case 0xD2:
			return data + 5 + ntohl(*(uint32_t *)(data + 1));
		case 0xD4: {
			int n = data[1];
			data = data + 2;
			for (uint32_t i = 0; i < n; i++) {
				data = bolt_value_read(data);
			}
			return data;
		}
		case 0xD5: {
			int n = *(uint16_t *)(data + 1);
			data = data + 3;
			for (uint32_t i = 0; i < n; i++) {
				data = bolt_value_read(data);
			}
			return data;
		}
		case 0xD6: {
			int n = *(uint32_t *)(data + 1);
			data = data + 5;
			for (uint32_t i = 0; i < n; i++) {
				data = bolt_value_read(data);
			}
			return data;
		}
		case 0xD8: {
			int n = data[1];
			data = data + 2;
			for (uint32_t i = 0; i < n; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			return data;
		}
		case 0xD9: {
			int n = *(uint16_t *)(data + 1);
			data = data + 3;
			for (uint32_t i = 0; i < n; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			return data;
		}
		case 0xDA: {
			int n = *(uint32_t *)(data + 1);
			data = data + 5;
			for (uint32_t i = 0; i < n; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			return data;
		}
		default:
			if(marker >= 0xF0 || marker <= 0x7F) {
				return data + 1;
			}
			ASSERT(false);
			break;
	}
}

bolt_value_type bolt_read_type
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xC0:
			return BVT_NULL;
		case 0xC1:
			return BVT_FLOAT;
		case 0xC2:
		case 0xC3:
			return BVT_BOOL;
		case 0xC8:
			return BVT_INT8;
		case 0xC9:
			return BVT_INT16;
		case 0xCA:
			return BVT_INT32;
		case 0xCB:
			return BVT_INT64;
		case 0xCC:
		case 0xCD:
		case 0xCE:
			return BVT_BYTES;
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
		case 0x8E:
		case 0x8F:
		case 0xD0:
		case 0xD1:
		case 0xD2:
			return BVT_STRING;
		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:
		case 0x9E:
		case 0x9F:
		case 0xD4:
		case 0xD5:
		case 0xD6:
			return BVT_LIST;
		case 0xA0:
		case 0xA1:
		case 0xA2:
		case 0xA3:
		case 0xA4:
		case 0xA5:
		case 0xA6:
		case 0xA7:
		case 0xA8:
		case 0xA9:
		case 0xAA:
		case 0xAB:
		case 0xAC:
		case 0xAD:
		case 0xAE:
		case 0xAF:
		case 0xD8:
		case 0xD9:
		case 0xDA:
			return BVT_MAP;
		case 0xB0:
		case 0xB1:
		case 0xB2:
		case 0xB3:
		case 0xB4:
		case 0xB5:
		case 0xB6:
		case 0xB7:
		case 0xB8:
		case 0xB9:
		case 0xBA:
		case 0xBB:
		case 0xBC:
		case 0xBD:
		case 0xBE:
		case 0xBF:
			return BVT_STRUCTURE;
		default:
			if(marker >= 0xF0 || marker <= 0x7F) {
				return BVT_INT8;
			}
			break;
	}
}

bool bolt_read_bool
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xC2:
			return false;
		case 0xC3:
			return true;
		default:
			ASSERT(false);
			return false;
	}
}

int8_t bolt_read_int8
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xC8:
			return data[1];
		default:
			if(marker >= 0xF0 || marker <= 0x7F) {
				return marker;
			}
			ASSERT(false);
			return 0;
	}
}

int16_t bolt_read_int16
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xC9:
			return ntohs(*(uint16_t *)(data + 1));
		default:
			ASSERT(false);
			return 0;
	}
}

int32_t bolt_read_int32
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xCA:
			return ntohl(*(uint32_t *)(data + 1));
		default:
			ASSERT(false);
			return 0;
	}
}

int64_t bolt_read_int64
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xCB:
			return ntohll(*(uint64_t *)(data + 1));
		default:
			ASSERT(false);
			return 0;
	}
}

double bolt_read_float
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xC1: {
			double d;
			char *buf = (char *)&d;
			for (int i = 0; i < sizeof(double); i++) {
				buf[i] = data[sizeof(double) - i];
			}
			return d;
		}
		default:
			ASSERT(false);
			return 0;
	}
}

uint32_t bolt_read_string_size
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
		case 0x8E:
		case 0x8F:
			return marker - 0x80;
		case 0xD0:
			return *(uint8_t *)(data + 1);
		case 0xD1:
			return ntohs(*(uint16_t *)(data + 1));
		case 0xD2:
			return ntohl(*(uint32_t *)(data + 1));
		default:
			ASSERT(false);
			return 0;
	}
}

char *bolt_read_string
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
		case 0x8E:
		case 0x8F:
			return data + 1;
		case 0xD0:
			return data + 2;
		case 0xD1:
			return data + 3;
		case 0xD2:
			return data + 5;
		default:
			ASSERT(false);
			return 0;
	}
}

uint32_t bolt_read_list_size
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:
		case 0x9E:
		case 0x9F:
			return marker - 0x90;
		case 0xD4:
			return *(uint8_t *)(data + 1);
		case 0xD5:
			return ntohs(*(uint16_t *)(data + 1));
		case 0xD6:
			return ntohl(*(uint32_t *)(data + 1));
		default:
			ASSERT(false);
			return 0;
	}
}

char *bolt_read_list_item
(
	char *data,
	uint32_t index
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:
		case 0x9E:
		case 0x9F:
			data = data + 1;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
			}
			return data;
		case 0xD4:
			data = data + 2;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
			}
			return data;
		case 0xD5:
			data = data + 3;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
			}
			return data;
		case 0xD6:
			data = data + 5;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
			}
			return data;
		default:
			ASSERT(false);
			return 0;
	}
}

uint32_t bolt_read_map_size
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xA0:
		case 0xA1:
		case 0xA2:
		case 0xA3:
		case 0xA4:
		case 0xA5:
		case 0xA6:
		case 0xA7:
		case 0xA8:
		case 0xA9:
		case 0xAA:
		case 0xAB:
		case 0xAC:
		case 0xAD:
		case 0xAE:
		case 0xAF:
			return marker - 0xA0;
		case 0xD8:
			return *(uint8_t *)(data + 1);
		case 0xD9:
			return ntohs(*(uint16_t *)(data + 1));
		case 0xDA:
			return ntohs(*(uint32_t *)(data + 1));
		default:
			ASSERT(false);
			return 0;
	}
}

char *bolt_read_map_key
(
	char *data,
	uint32_t index
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xA0:
		case 0xA1:
		case 0xA2:
		case 0xA3:
		case 0xA4:
		case 0xA5:
		case 0xA6:
		case 0xA7:
		case 0xA8:
		case 0xA9:
		case 0xAA:
		case 0xAB:
		case 0xAC:
		case 0xAD:
		case 0xAE:
		case 0xAF:
			data = data + 1;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			return data;
		case 0xD8:
			data = data + 2;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			return data;
		case 0xD9:
			data = data + 3;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			return data;
		case 0xDA:
			data = data + 5;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			return data;
		default:
			ASSERT(false);
			return 0;
	}
}

char *bolt_read_map_value
(
	char *data,
	uint32_t index
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xA0:
		case 0xA1:
		case 0xA2:
		case 0xA3:
		case 0xA4:
		case 0xA5:
		case 0xA6:
		case 0xA7:
		case 0xA8:
		case 0xA9:
		case 0xAA:
		case 0xAB:
		case 0xAC:
		case 0xAD:
		case 0xAE:
		case 0xAF:
			data = data + 1;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			data = bolt_value_read(data);
			return data;
		case 0xD8:
			data = data + 2;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			data = bolt_value_read(data);
			return data;
		case 0xD9:
			data = data + 3;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			data = bolt_value_read(data);
			return data;
		case 0xDA:
			data = data + 5;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
				data = bolt_value_read(data);
			}
			data = bolt_value_read(data);
			return data;
		default:
			ASSERT(false);
			return 0;
	}
}

bolt_structure_type bolt_read_structure_type
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xB0:
		case 0xB1:
		case 0xB2:
		case 0xB3:
		case 0xB4:
		case 0xB5:
		case 0xB6:
		case 0xB7:
		case 0xB8:
			return data[1];
		default:
			ASSERT(false);
			return 0;
	}
}

uint32_t bolt_read_structure_size
(
	char *data
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xB0:
		case 0xB1:
		case 0xB2:
		case 0xB3:
		case 0xB4:
		case 0xB5:
		case 0xB6:
		case 0xB7:
		case 0xB8:
		case 0xB9:
		case 0xBA:
		case 0xBB:
		case 0xBC:
		case 0xBD:
		case 0xBE:
		case 0xBF:
			return marker - 0xB0;
		default:
			ASSERT(false);
			return 0;
	}
}

char *bolt_read_structure_value
(
	char *data,
	uint32_t index
) {
	uint8_t marker = data[0];
	switch (marker)
	{
		case 0xB0:
		case 0xB1:
		case 0xB2:
		case 0xB3:
		case 0xB4:
		case 0xB5:
		case 0xB6:
		case 0xB7:
		case 0xB8:
		case 0xB9:
		case 0xBA:
		case 0xBB:
		case 0xBC:
		case 0xBD:
		case 0xBE:
		case 0xBF:
			data = data + 2;
			for (uint32_t i = 0; i < index; i++) {
				data = bolt_value_read(data);
			}
			return data;
		default:
			ASSERT(false);
			return 0;
	}
}
