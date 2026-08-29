#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ===========================
// AI-Thinker Pin Definitions
// ===========================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_GPIO_NUM       4 

const char* ssid = "Airtel_Node";
const char* password = "air66343";

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t camera_httpd = NULL;
WiFiUDP udp;

void setupLedFlash(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

// UDP Broadcast Task for App Discovery
void udpBroadcastTask(void *pvParameters) {
    const IPAddress multicast_ip(224, 0, 0, 251);
    const uint16_t multicast_port = 5353;
    
    while(true) {
        if(WiFi.status() == WL_CONNECTED) {
            udp.beginPacket(multicast_ip, multicast_port);
            // Payload signature that app listens for
            udp.printf("_camctrl._udp.local drv=ESP32-CAM ip=%s", WiFi.localIP().toString().c_str());
            udp.endPacket();
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Broadcast every 1 second
    }
}

// MJPEG Stream Handler with Strict 3 FPS FreeRTOS Pacing
static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];
    
    // Enforce exactly 333ms interval (3 FPS) to guarantee network recovery
    const TickType_t xTargetTicks = pdMS_TO_TICKS(333);
    TickType_t xLastWakeTime;

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) return res;

    xLastWakeTime = xTaskGetTickCount();

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("FB acquisition fault");
            res = ESP_FAIL;
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }
        
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        
        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } 
        
        if(res != ESP_OK) {
            Serial.printf("TCP Socket collapsed. LwIP Error: 0x%x\n", res);
            break;
        }

        // RTOS Yield: Dictates the 3 FPS temporal pacing
        vTaskDelayUntil(&xLastWakeTime, xTargetTicks);
    }
    return res;
}

static esp_err_t cmd_handler(httpd_req_t *req) {
    char*  buf;
    size_t buf_len;
    char variable[32] = {0,};
    char value[32] = {0,};

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf) return httpd_resp_send_500(req);
        
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) != ESP_OK ||
                httpd_query_key_value(buf, "val", value, sizeof(value)) != ESP_OK) {
                free(buf);
                return httpd_resp_send_404(req);
            }
        }
        free(buf);
    } else {
        return httpd_resp_send_404(req);
    }

    if(!strcmp(variable, "led")) {
        int val = atoi(value);
        digitalWrite(LED_GPIO_NUM, val ? HIGH : LOW);
        Serial.printf("Actuating illumination diode: %d\n", val);
    }
    else if(!strcmp(variable, "uart")) {
        Serial1.printf("%s\n", value);
        Serial.printf("UART relay transmission executing: %s\n", value);
    }
    else {
        return httpd_resp_send_404(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, "Command Execution Acknowledged", HTTPD_RESP_USE_STRLEN);
}

void startCameraServer(){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.server_port = 80;
    
    // CRITICAL FIX: Prevent the socket from blocking for more than 1 second
    config.send_wait_timeout = 1; 

    httpd_uri_t stream_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t cmd_uri = {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = NULL
    };

    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &stream_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        Serial.println("Asynchronous HTTP daemon instantiated.");
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, 15, 14); 
    
    Serial.setDebugOutput(true);
    Serial.println();

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    
    config.xclk_freq_hz = 20000000; 
    
    // DOWNGRADE to VGA (640x480) for robust physical layer transmission
    config.frame_size = FRAMESIZE_VGA;
    config.pixel_format = PIXFORMAT_JPEG; 
    
    if(psramFound()){
        // INCREASED integer value to 15 (higher compression, smaller payload)
        config.jpeg_quality = 15;
        config.fb_count = 2; 
        config.grab_mode = CAMERA_GRAB_LATEST; 
        config.fb_location = CAMERA_FB_IN_PSRAM;
    } else {
        config.frame_size = FRAMESIZE_QVGA;
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.fb_count = 1;
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Hardware initialization failed. Hexadecimal fault code: 0x%x", err);
        return;
    }

    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1); 
        s->set_brightness(s, 1); 
        s->set_saturation(s, -2); 
    }

    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);

    setupLedFlash(LED_GPIO_NUM);

    WiFi.begin(ssid, password);
    WiFi.setSleep(false); 

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nNetwork interface established.");

    startCameraServer();
    
    xTaskCreate(udpBroadcastTask, "udpBroadcastTask", 4096, NULL, 1, NULL);

    Serial.print("Video pipeline initialized. Access daemon via: http://");
    Serial.print(WiFi.localIP());
    Serial.println("");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(10000));
}
