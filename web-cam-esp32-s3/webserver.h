#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_http_server.h"
#include "esp_camera.h"

// HTTP Server handle
extern httpd_handle_t camera_httpd;

// Initialize and start HTTP server
void startCameraServer();

// HTTP request handlers
esp_err_t index_handler(httpd_req_t *req);
esp_err_t stream_handler(httpd_req_t *req);

#endif // WEBSERVER_H
