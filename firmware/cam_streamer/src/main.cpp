#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ============================================================
// AI-Thinker ESP32-CAM Pin Definitions
// ============================================================
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

// ============================================================
// Wi-Fi
// ============================================================
const char* ssid     = "Airtel_Node";
const char* password = "air66343";

// ============================================================
// MJPEG Stream Definitions
// ============================================================
#define PART_BOUNDARY "123456789000000000000987654321"

static const char* STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;

static const char* STREAM_BOUNDARY =
    "\r\n--" PART_BOUNDARY "\r\n";

static const char* STREAM_PART =
    "Content-Type: image/jpeg\r\n"
    "Content-Length: %u\r\n\r\n";

httpd_handle_t camera_httpd = NULL;

// ============================================================
// LED
// ============================================================
void setupLedFlash(int pin)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

// ============================================================
// Camera Diagnostics
// ============================================================
void printSystemStatus()
{
    Serial.println();
    Serial.println("========== SYSTEM STATUS ==========");

    Serial.printf(
        "PSRAM detected : %s\n",
        psramFound() ? "YES" : "NO"
    );

    Serial.printf(
        "PSRAM size     : %u bytes\n",
        ESP.getPsramSize()
    );

    Serial.printf(
        "Free PSRAM     : %u bytes\n",
        ESP.getFreePsram()
    );

    Serial.printf(
        "Free heap      : %u bytes\n",
        ESP.getFreeHeap()
    );

    Serial.printf(
        "WiFi RSSI      : %d dBm\n",
        WiFi.RSSI()
    );

    Serial.println("===================================");
    Serial.println();
}

// ============================================================
// MJPEG Stream Handler
// Maximum target rate: ~3 FPS
// ============================================================
static esp_err_t stream_handler(httpd_req_t* req)
{
    esp_err_t res = httpd_resp_set_type(
        req,
        STREAM_CONTENT_TYPE
    );

    if (res != ESP_OK) {
        Serial.printf(
            "HTTP header error: %s (0x%x)\n",
            esp_err_to_name(res),
            res
        );

        return res;
    }

    char part_buf[64];

    Serial.println("Client connected to video stream.");

    TickType_t lastFrameTime = xTaskGetTickCount();

    while (true)
    {
        // ----------------------------------------------------
        // Acquire camera frame
        // ----------------------------------------------------
        camera_fb_t* fb = esp_camera_fb_get();

        if (fb == nullptr)
        {
            Serial.println(
                "CAMERA FAILURE: esp_camera_fb_get() returned NULL"
            );

            // Do not immediately kill the HTTP connection.
            vTaskDelay(pdMS_TO_TICKS(100));

            continue;
        }

        // ----------------------------------------------------
        // JPEG information
        // ----------------------------------------------------
        const size_t jpg_len = fb->len;
        const uint8_t* jpg_buf = fb->buf;

        // ----------------------------------------------------
        // Send MJPEG frame header
        // ----------------------------------------------------
        size_t hlen = snprintf(
            part_buf,
            sizeof(part_buf),
            STREAM_PART,
            jpg_len
        );

        res = httpd_resp_send_chunk(
            req,
            part_buf,
            hlen
        );

        // ----------------------------------------------------
        // Send JPEG
        // ----------------------------------------------------
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(
                req,
                (const char*)jpg_buf,
                jpg_len
            );
        }

        // ----------------------------------------------------
        // Send frame boundary
        // ----------------------------------------------------
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(
                req,
                STREAM_BOUNDARY,
                strlen(STREAM_BOUNDARY)
            );
        }

        // ----------------------------------------------------
        // Return framebuffer immediately
        // ----------------------------------------------------
        esp_camera_fb_return(fb);
        fb = nullptr;

        // ----------------------------------------------------
        // Network / HTTP failure
        // ----------------------------------------------------
        if (res != ESP_OK)
        {
            Serial.printf(
                "HTTP STREAM FAILURE: %s (0x%x)\n",
                esp_err_to_name(res),
                res
            );

            break;
        }

        // ----------------------------------------------------
        // Keep stream around 3 FPS maximum
        // ----------------------------------------------------
        vTaskDelayUntil(
            &lastFrameTime,
            pdMS_TO_TICKS(333)
        );
    }

    Serial.println("Video client disconnected.");

    return res;
}

// ============================================================
// Command Handler
//
// Examples:
//
// /control?var=led&val=1
// /control?var=led&val=0
// /control?var=uart&val=HELLO
// ============================================================
static esp_err_t cmd_handler(httpd_req_t* req)
{
    char* buf = nullptr;

    size_t buf_len =
        httpd_req_get_url_query_len(req) + 1;

    char variable[32] = {0};
    char value[32] = {0};

    if (buf_len <= 1)
    {
        return httpd_resp_send_404(
            req
        );
    }

    buf = (char*)malloc(buf_len);

    if (buf == nullptr)
    {
        return httpd_resp_send_500(
            req
        );
    }

    esp_err_t queryResult =
        httpd_req_get_url_query_str(
            req,
            buf,
            buf_len
        );

    if (queryResult != ESP_OK)
    {
        free(buf);

        return httpd_resp_send_404(
            req
        );
    }

    if (
        httpd_query_key_value(
            buf,
            "var",
            variable,
            sizeof(variable)
        ) != ESP_OK
        ||
        httpd_query_key_value(
            buf,
            "val",
            value,
            sizeof(value)
        ) != ESP_OK
    )
    {
        free(buf);

        return httpd_resp_send_404(
            req
        );
    }

    free(buf);

    // ========================================================
    // LED Control
    // ========================================================
    if (strcmp(variable, "led") == 0)
    {
        int val = atoi(value);

        digitalWrite(
            LED_GPIO_NUM,
            val ? HIGH : LOW
        );

        Serial.printf(
            "LED command: %d\n",
            val
        );
    }

    // ========================================================
    // UART Relay
    // ========================================================
    else if (strcmp(variable, "uart") == 0)
    {
        Serial1.printf(
            "%s\n",
            value
        );

        Serial.printf(
            "UART command: %s\n",
            value
        );
    }

    else
    {
        return httpd_resp_send_404(
            req
        );
    }

    httpd_resp_set_hdr(
        req,
        "Access-Control-Allow-Origin",
        "*"
    );

    return httpd_resp_send(
        req,
        "Command Execution Acknowledged",
        HTTPD_RESP_USE_STRLEN
    );
}

// ============================================================
// Start HTTP Server
// ============================================================
void startCameraServer()
{
    httpd_config_t config =
        HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = 8;
    config.server_port = 80;

    // Keep send timeout reasonably short so a dead client
    // does not hold the stream task indefinitely.
    config.send_wait_timeout = 1;

    // --------------------------------------------------------
    // Stream endpoint
    // GET /
    // --------------------------------------------------------
    httpd_uri_t stream_uri =
    {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = nullptr
    };

    // --------------------------------------------------------
    // Command endpoint
    // GET /control
    // --------------------------------------------------------
    httpd_uri_t cmd_uri =
    {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = nullptr
    };

    // --------------------------------------------------------
    // Start server
    // --------------------------------------------------------
    esp_err_t err =
        httpd_start(
            &camera_httpd,
            &config
        );

    if (err != ESP_OK)
    {
        Serial.printf(
            "HTTP server startup failed: %s (0x%x)\n",
            esp_err_to_name(err),
            err
        );

        return;
    }

    httpd_register_uri_handler(
        camera_httpd,
        &stream_uri
    );

    httpd_register_uri_handler(
        camera_httpd,
        &cmd_uri
    );

    Serial.println(
        "HTTP server started successfully."
    );
}

// ============================================================
// Setup
// ============================================================
void setup()
{
    Serial.begin(115200);

    // UART relay
    Serial1.begin(
        115200,
        SERIAL_8N1,
        15,
        14
    );

    Serial.setDebugOutput(true);

    Serial.println();
    Serial.println(
        "======================================"
    );
    Serial.println(
        " AI-Thinker ESP32-CAM Boot"
    );
    Serial.println(
        "======================================"
    );

    // ========================================================
    // Camera Configuration
    // ========================================================
    camera_config_t config;

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

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
    config.pin_href  = HREF_GPIO_NUM;

    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    config.pin_pwdn  = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;

    config.pixel_format = PIXFORMAT_JPEG;

    // ========================================================
    // Conservative initial configuration
    //
    // Start at QVGA to establish stability.
    // Once stable, change to FRAMESIZE_VGA.
    // ========================================================
    config.frame_size = FRAMESIZE_QVGA;

    if (psramFound())
    {
        config.jpeg_quality = 15;

        config.fb_count = 2;

        config.grab_mode =
            CAMERA_GRAB_WHEN_EMPTY;

        config.fb_location =
            CAMERA_FB_IN_PSRAM;
    }
    else
    {
        config.jpeg_quality = 18;

        config.fb_count = 1;

        config.grab_mode =
            CAMERA_GRAB_WHEN_EMPTY;

        config.fb_location =
            CAMERA_FB_IN_DRAM;
    }

    // ========================================================
    // Print memory information
    // ========================================================
    printSystemStatus();

    // ========================================================
    // Initialize camera
    // ========================================================
    Serial.println(
        "Initializing camera..."
    );

    esp_err_t err =
        esp_camera_init(&config);

    if (err != ESP_OK)
    {
        Serial.printf(
            "CAMERA INITIALIZATION FAILED: %s (0x%x)\n",
            esp_err_to_name(err),
            err
        );

        return;
    }

    Serial.println(
        "Camera initialized successfully."
    );

    // ========================================================
    // Camera Sensor Configuration
    // ========================================================
    sensor_t* s =
        esp_camera_sensor_get();

    if (s != nullptr)
    {
        // Vertical flip
        s->set_vflip(
            s,
            1
        );

        // Horizontal mirror
        s->set_hmirror(
            s,
            1
        );

        // Slightly reduce saturation
        s->set_saturation(
            s,
            -2
        );

        Serial.printf(
            "Sensor PID: 0x%04X\n",
            s->id.PID
        );
    }

    // ========================================================
    // Flash LED
    // ========================================================
    setupLedFlash(
        LED_GPIO_NUM
    );

    // ========================================================
    // Wi-Fi
    // ========================================================
    Serial.println(
        "Connecting to Wi-Fi..."
    );

    WiFi.mode(
        WIFI_STA
    );

    WiFi.setSleep(
        false
    );

    WiFi.begin(
        ssid,
        password
    );

    while (
        WiFi.status() != WL_CONNECTED
    )
    {
        delay(500);

        Serial.print(".");
    }

    Serial.println();
    Serial.println(
        "Wi-Fi connection established."
    );

    Serial.print(
        "IP address: "
    );

    Serial.println(
        WiFi.localIP()
    );

    Serial.printf(
        "Wi-Fi RSSI: %d dBm\n",
        WiFi.RSSI()
    );

    // ========================================================
    // Start HTTP Server
    // ========================================================
    startCameraServer();

    Serial.println();
    Serial.println(
        "======================================"
    );
    Serial.println(
        " VIDEO PIPELINE READY"
    );
    Serial.println(
        "======================================"
    );

    Serial.print(
        "Stream URL: http://"
    );

    Serial.println(
        WiFi.localIP()
    );

    Serial.print(
        "Control URL: http://"
    );

    Serial.print(
        WiFi.localIP()
    );

    Serial.println(
        "/control"
    );

    Serial.println();
}

// ============================================================
// Main Loop
// ============================================================
void loop()
{
    // Periodic diagnostics.
    static unsigned long lastStatus = 0;

    if (
        millis() - lastStatus >= 10000
    )
    {
        lastStatus = millis();

        Serial.printf(
            "[STATUS] Heap: %u | PSRAM: %u | WiFi: %d dBm\n",
            ESP.getFreeHeap(),
            ESP.getFreePsram(),
            WiFi.RSSI()
        );
    }

    delay(100);
}