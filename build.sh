#!/bin/bash

# 打印机管理器GUI构建脚本

echo "=== 打印机管理器GUI构建脚本 ==="

# 检查Qt是否安装
if command -v qmake &> /dev/null; then
    echo "使用qmake构建..."
    qmake PrintManager.pro
    if [ $? -eq 0 ]; then
        make
        if [ $? -eq 0 ]; then
            echo "构建成功！运行 ./PrintManagerGUI 启动程序"
        else
            echo "构建失败！"
            exit 1
        fi
    else
        echo "qmake配置失败！"
        exit 1
    fi
elif command -v cmake &> /dev/null; then
    echo "使用CMake构建..."
    mkdir -p build
    cd build
    cmake ..
    if [ $? -eq 0 ]; then
        make
        if [ $? -eq 0 ]; then
            echo "构建成功！运行 ./build/PrintManagerGUI 启动程序"
        else
            echo "构建失败！"
            exit 1
        fi
    else
        echo "CMake配置失败！"
        exit 1
    fi
else
    echo "错误：未找到qmake或cmake！"
    echo "请安装Qt开发工具："
    echo "  Ubuntu/Debian: sudo apt-get install qt5-default qtbase5-dev"
    echo "  或: sudo apt-get install qt6-base-dev"
    exit 1
fi

