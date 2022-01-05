#pragma once
#include <coap3/coap.h>

int resolve_address(const char *host, const char *service, coap_address_t *dst);
