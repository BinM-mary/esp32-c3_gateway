# espweblink 项目文档

## 一、项目概述

`espweblink` 是一个基于 ESP32 的项目，主要用于实现 WiFi 扫描、配置和建立连接，同时包含一个 Web 服务器和 DNS 服务器，支持通过 Web 页面进行 WiFi 配置。项目利用 ESP-IDF 框架开发，具备 WiFi Station 和 SoftAP 模式，可自动重连 WiFi，支持将 WiFi 配置信息存储到 NVS（非易失性存储）中。

## 二、项目结构

```plaintext
espweblink/
├── main/
│   ├── main.c              # 主程序文件，包含WiFi初始化、事件处理、HTTP和DNS服务器启动等功能
│   ├── dns_server.c        # DNS服务器相关代码，用于解析DNS请求并响应
│   ├── scan.c              # WiFi扫描功能实现，可获取附近WiFi列表并生成JSON数据
│   ├── root.html           # 前端Web页面，用于展示WiFi列表和进行配置
│   └── CMakeLists.txt      # 主模块CMake配置文件
├── pytest_captive_portal.py  # 测试脚本，用于测试重定向和捕获页面功能
├── CMakeLists.txt          # 项目CMake配置文件
```

## 三、功能特性

1. **WiFi 扫描**：周期性扫描附近的 WiFi 网络，并将扫描结果存储在全局变量中，可通过 JSON 格式获取。
2. **WiFi 配置**：支持通过 Web 页面输入 WiFi 的 SSID 和密码进行配置，配置信息会存储到 NVS 中。
3. **自动重连**：在 WiFi 连接失败时，会自动尝试重新连接，达到最大重试次数后会切换到 SoftAP 模式。
4. **Web 服务器**：提供 HTTP 服务，可访问根页面和获取 WiFi 列表，支持处理 WiFi 配置的 POST 请求。
5. **DNS 服务器**：解析 DNS 请求，将所有请求重定向到 SoftAP 的 IP 地址。
6. **测试脚本**：包含测试重定向和捕获页面功能的脚本，可验证项目的基本功能。

## 四、环境搭建

### 4.1 安装 ESP-IDF

请按照 ESP-IDF 官方文档的指引，完成 ESP-IDF 的安装和环境配置。


### 4.2 配置项目

根据实际需求，在 main.c 文件中修改以下配置项：

```c
#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN
#define EXAMPLE_ESP_MAXIMUM_RETRY  5
```

### 4.3 编译和烧录

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash  # 根据实际情况修改串口设备路径
```

## 五、使用说明

### 5.1 启动项目

烧录完成后，ESP32 会自动启动，尝试连接到之前配置的 WiFi 网络。如果连接失败，会切换到 SoftAP 模式。

### 5.2 访问 Web 页面

在 SoftAP 模式下，将设备连接到 ESP32 创建的 WiFi 网络，然后在浏览器中输入 SoftAP 的 IP 地址，即可访问配置页面。

### 5.3 配置 WiFi

在 Web 页面上，选择要连接的 WiFi 网络，输入密码，点击保存即可完成配置。配置信息会存储到 NVS 中，下次启动时会自动使用。

### 5.4 测试功能

可以运行 pytest_captive_portal.py 脚本进行功能测试，验证重定向和捕获页面功能是否正常。

## 六、代码说明

### 6.1 main.c

- `wifi_init()`：初始化 WiFi，包括 NVS、网络接口和事件循环。
- `wifi_init_sta()`：初始化 WiFi Station 模式，尝试连接到指定的 WiFi 网络。
- `wifi_init_softapsta()`：初始化 SoftAP 和 Station 模式，创建 WiFi 热点。
- `root_get_handler()`：处理 HTTP GET 请求，返回根页面。
- `wifi_list_get_handler()`：处理 HTTP GET 请求，返回 WiFi 列表的 JSON 数据。
- `wifi_config_post_handler()`：处理 HTTP POST 请求，接收 WiFi 配置信息并保存到 NVS 中。

### 6.2 scan.c

- `wifi_scan_task()`：周期性扫描附近的 WiFi 网络，并将结果存储在全局变量中。
- `get_wifi_list_json()`：将扫描到的 WiFi 列表转换为 JSON 格式的字符串。

### 6.3 dns_server.c

- `parse_dns_name()`：解析 DNS 请求中的域名。
- `parse_dns_request()`：解析 DNS 请求并生成响应，将所有请求重定向到 SoftAP 的 IP 地址。

## 七、注意事项

- 请确保 ESP-IDF 环境配置正确，编译和烧录过程中可能会出现依赖问题，需要根据错误信息进行解决。
- 在使用测试脚本时，需要确保主机能够连接到 ESP32 创建的 WiFi 网络，并且具备相应的 WiFi 接口。
- 存储在 NVS 中的 WiFi 配置信息可以在设备重启后保留，但如果需要清除配置信息，可以通过代码或 NVS 工具进行操作。