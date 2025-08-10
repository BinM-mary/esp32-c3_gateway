#!/bin/bash

# ESP-IDF环境设置脚本
echo "正在设置ESP-IDF环境..."

# 检查ESP-IDF路径
if [ -z "$IDF_PATH" ]; then
    # 尝试常见的ESP-IDF安装路径
    if [ -d "$HOME/esp-idf" ]; then
        export IDF_PATH="$HOME/esp-idf"
    elif [ -d "/opt/esp-idf" ]; then
        export IDF_PATH="/opt/esp-idf"
    else
        echo "错误: 未找到ESP-IDF，请确保已安装ESP-IDF"
        echo "请运行: git clone --recursive https://github.com/espressif/esp-idf.git"
        exit 1
    fi
fi

# 设置ESP-IDF环境
if [ -f "$IDF_PATH/export.sh" ]; then
    source "$IDF_PATH/export.sh"
    echo "ESP-IDF环境已设置: $IDF_PATH"
else
    echo "错误: 未找到ESP-IDF export.sh文件"
    exit 1
fi

# 显示环境信息
echo "IDF_PATH: $IDF_PATH"
echo "IDF_TARGET: $IDF_TARGET"
echo "PATH: $PATH" | head -1

echo "环境设置完成！"
echo "现在可以运行: idf.py build" 