#include "pill.h"

typedef enum DevicePillState {
	PILL_BOX_CONFIRMED,
} DevicePillState;

typedef struct PutDeviceIpData {
	char * device_ip;
} PutDeviceIpData;

typedef struct PostDevicePillData {
	int position;
	Pill * pill;
} PostDevicePillData;

typedef struct PutDevicePillStateData {
	char * device_pill_key;
	DevicePillState state;
} PutDevicePillStateData;

typedef struct PostData {
	union {
		PutDeviceIpData put_device_ip_data;
		PostDevicePillData post_device_pill_data;
		PutDevicePillStateData put_device_pill_state_data;
	};
} PostData;

typedef enum ApiEndpoint {
	PUT_DEVICE_IP,
	POST_DEVICE_PILL,
	PUT_DEVICE_PILL_STATUS,
} ApiEndpoint;

void init_api_client_task();

int api_make_request(ApiEndpoint endpoint, PostData data);

void put_device_ip(char device_ip[]);
void post_device_pill(int position, Pill * pill);
void put_device_pill_state(char * pill_key, DevicePillState state);
