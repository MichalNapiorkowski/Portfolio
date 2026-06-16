#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "esp_http_server.h"
#include "camera_index.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Arduino.h>
#include "html_page.h"

#define UART2_TX_PIN 2
#define UART2_RX_PIN 15
#define UART1_TX_PIN 33
#define UART1_RX_PIN 32
#define R 13
#define G 14
#define B 12

#define CAMERA_MODEL_WROVER_KIT
#include "camera_pins.h"

const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

volatile int button_action = 0;
volatile bool ledState = false;
unsigned long lastSerialTime = 0;
const unsigned long serialInterval = 180;

int sensSlider = 0;
int distance = -1;
char receivedChar = '0';
char distanceChar = '+';
int wifiBars = 0;  
char enable = 'F';
int batteryBars = 0;

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[128];

    res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    if (res != ESP_OK) {
      return res;
    }

    while (true) {
      fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Camera capture failed");
        res = ESP_FAIL;
      } else {
        if (fb->format != PIXFORMAT_JPEG) {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted) {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }

      if (res == ESP_OK) {
        size_t hlen = snprintf((char *)part_buf, 128,
                               "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                               _jpg_buf_len);
        res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
      }

      if (res == ESP_OK) {
        res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
      }

      if (res == ESP_OK) {
        res = httpd_resp_send_chunk(req, "\r\n", 2);
      }

      if (fb) {
        esp_camera_fb_return(fb);
        fb = NULL;
        _jpg_buf = NULL;
      } else if (_jpg_buf) {
        free(_jpg_buf);
        _jpg_buf = NULL;
      }

      if (res != ESP_OK) {
        break;
      }
    }

    return res;
}

static esp_err_t signal_handler(httpd_req_t* req) {
    char buff[4];
    sprintf(buff, "%d", wifiBars); 
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, buff, strlen(buff));
}

static esp_err_t button_handler(httpd_req_t* req) {
    char* buf;
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;

    if (buf_len > 0) {
        buf = (char*)malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (strstr(buf, "action=up")) {
                button_action = 1;
            } else if (strstr(buf, "action=down")) {
                if(enable == 'T') button_action = 2;
            } else if (strstr(buf, "action=left")) {
                button_action = 3;
            } else if (strstr(buf, "action=right")) {
                button_action = 4;
            } else if (strstr(buf, "action=speed1")) {
                button_action = 5;
            } else if (strstr(buf, "action=speed2")) {
                button_action = 6;
            } else if (strstr(buf, "action=speed3")) {
                button_action = 7;
            } else if (strstr(buf, "action=speed4")) {
                button_action = 8;
            } else if (strstr(buf, "action=speed5")) {
                button_action = 9;
            } else if (strstr(buf, "action=disable")) {
                button_action = 10;
            } else if (strstr(buf, "action=enable")) {
                button_action = 11;
            } else if (strstr(buf, "action=sens0")) {
                button_action = 12;
            } else if (strstr(buf, "action=sens1")) {
                button_action = 13;
            } else if (strstr(buf, "action=sens2")) {
                button_action = 14;
            } else if (strstr(buf, "action=sens3")) {
                button_action = 15;
            } else if (strstr(buf, "action=sens4")) {
                button_action = 16;
            } else if (strstr(buf, "action=sens5")) {
                button_action = 17;
            } else if (strstr(buf, "action=sens6")) {
                button_action = 18;
            } else if (strstr(buf, "action=sens7")) {
                button_action = 19;
            } else if (strstr(buf, "action=sens8")) {
                button_action = 20;
            } else if (strstr(buf, "action=sens9")) {
                button_action = 21;
            }
        }
        free(buf);
    }

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t index_handler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html_page, strlen(html_page));
}

static esp_err_t led_status_handler(httpd_req_t* req) {
    const char* status = ledState ? "ON" : "OFF";
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, status, strlen(status));
}

static esp_err_t distance_handler(httpd_req_t* req) {
    char distStr[4];
    sprintf(distStr, "%d", distance); 
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, distStr, strlen(distStr));
}

static esp_err_t battery_handler(httpd_req_t* req) {
    char buff[4];
    sprintf(buff, "%d", batteryBars);
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, buff, strlen(buff));
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
      .user_ctx = NULL
    };

    httpd_uri_t stream_uri = {
      .uri = "/stream",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL
    };

    httpd_uri_t button_uri = {
      .uri = "/button",
      .method = HTTP_GET,
      .handler = button_handler,
      .user_ctx = NULL
    };

    httpd_uri_t led_status_uri = {
      .uri = "/led-status",
      .method = HTTP_GET,
      .handler = led_status_handler,
      .user_ctx = NULL
    };

    httpd_uri_t distance_uri = {
      .uri = "/distance",
      .method = HTTP_GET,
      .handler = distance_handler,
      .user_ctx = NULL
    };

    httpd_uri_t signal_uri = {
      .uri = "/signal",
      .method = HTTP_GET,
      .handler = signal_handler,
      .user_ctx = NULL
    };

    httpd_uri_t battery_uri = {
      .uri = "/battery",
      .method = HTTP_GET,
      .handler = battery_handler,
      .user_ctx = NULL
    };

    httpd_register_uri_handler(camera_httpd, &battery_uri);

    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &button_uri);
        httpd_register_uri_handler(camera_httpd, &led_status_uri);
        httpd_register_uri_handler(camera_httpd, &distance_uri);
        httpd_register_uri_handler(camera_httpd, &signal_uri);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.println("Camera HTTP server started");
    }
}

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);
    Serial2.begin(9600, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);
    Serial.println();
    analogReadResolution(10);

    pinMode(R, OUTPUT);
    pinMode(G, OUTPUT);
    pinMode(B, OUTPUT);

    digitalWrite(R, LOW);
    digitalWrite(G, LOW);
    digitalWrite(B, LOW);

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
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_LATEST; 

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
      Serial.printf("Camera init failed with error 0x%x", err);
      return;
    }

    sensor_t *s = esp_camera_sensor_get();
    // s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, 2);
    s->set_contrast(s, 2);

    WiFi.begin(ssid, password);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.setSleep(false);
    while (WiFi.status() != WL_CONNECTED) {
      digitalWrite(B, HIGH);
      delay(300);
      digitalWrite(B, LOW);
      delay(300);
      Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("Camera Stream Ready! Go to: http://");
    Serial.println(WiFi.localIP());

    startCameraServer();
}

void loop() {
    static unsigned long lastSignalCheck = 0;
    unsigned long currentTime = millis();

    if (currentTime - lastSerialTime >= serialInterval) {
      if (button_action >= 10 && button_action <= 11) {
        enable = (button_action == 10) ? 'T' : 'F';
        Serial2.print(enable);
        lastSerialTime = currentTime;
        button_action = 0;
      } else if (button_action >= 5 && button_action <= 9) {
        char speedChar = 'a' + (button_action - 5);
        Serial2.print(speedChar);
        lastSerialTime = currentTime;
        button_action = 0;
      } else if (button_action > 0 && button_action < 5) {
        Serial2.print(button_action);
        lastSerialTime = currentTime;
        button_action = 0;
      }
    }

    if (currentTime - lastSignalCheck >= 1000) {
      lastSignalCheck = currentTime;
      int32_t rssi = WiFi.RSSI(); 
      if      (rssi > -50) wifiBars = 5;
      else if (rssi > -60) wifiBars = 4;
      else if (rssi > -67) wifiBars = 3;
      else if (rssi > -72) wifiBars = 2;
      else if (rssi > -80) wifiBars = 1;
      else                 wifiBars = 0;
    }

    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(B, HIGH);
      delay(300);
      digitalWrite(B, LOW);
      delay(300);
    } else {
      digitalWrite(B, HIGH);
    }
}