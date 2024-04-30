typedef struct PostDeviceIpData {
	char * device_ip;
} PostDeviceIpData;

typedef struct PostData {
	union {
		PostDeviceIpData post_device_ip_data;
	};
} PostData;

typedef enum ApiEndpoint {
	POST_DEVICE_IP
} ApiEndpoint;

void init_api_client_task();

int api_make_request(ApiEndpoint endpoint, PostData data);

void post_device_ip(char device_ip[]);
