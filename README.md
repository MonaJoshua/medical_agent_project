# 🏥 智能就医助手

> 专为老年人/行动不便人群设计的智慧医疗语音交互系统

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-5.15+-green.svg)](https://www.qt.io/)
[![Python](https://img.shields.io/badge/Python-3.8+-blue.svg)](https://www.python.org/)
[![Flask](https://img.shields.io/badge/Flask-3.0-black.svg)](https://flask.palletsprojects.com/)

---

## 📖 项目简介

**智能就医助手** 是一套面向老年及行动不便人群的智慧医疗全流程服务系统。以**语音交互**为核心入口，整合 AI 大模型分诊、自助挂号、生命体征监护及医院管理后台，实现从“说出症状”到“挂号成功”的完整就医闭环。

**核心价值**：通过语音交互大幅降低老年人使用数字医疗服务的门槛，将通义千问大模型能力落地到具体医疗场景，提供终端、后端、管理后台三位一体的完整解决方案。

---

## ✨ 功能特性

### 🎤 语音助手
- 语音识别（STT）+ 语音合成（TTS）完整闭环
- 基于通义千问的多轮健康咨询
- 智能意图识别（挂号/查号源/查记录/帮助）
- 支持语音与文字双模交互

### 📋 一键挂号
- 科室网格展示，支持语音/点击选择
- 号源实时查询与防重复校验
- 挂号结果卡片展示（票号/排队号）

### 🫀 生命体征监护仪
- 实时显示心率、血氧、体温
- QPainter 动态波形绘制
- 可自定义报警阈值，异常时报警灯闪烁
- 历史数据存储与趋势图查看

### 🖥️ 医院管理后台
- 科室/医生/号源 CRUD 管理
- 挂号记录查看、筛选、状态修改
- CSV 数据导出
- 数据看板（热门科室 TOP5、挂号趋势图）
- 终端管理与 API Key 认证

---

## 🏗️ 系统架构

```mermaid
flowchart TD
    subgraph 用户层["🖥️ 用户层 (Qt/C++ 桌面终端)"]
        A[🎤 语音助手]
        B[📋 一键挂号]
        C[🫀 监护仪]
        D[🔧 更多工具]
    end

    subgraph API层["🌐 API 层"]
        API[⬇ HTTP / JSON RESTful API]
    end

    subgraph 服务层["⚙️ 服务层 (Python/Flask)"]
        E[/api/chat<br>健康咨询]
        F[/api/triage<br>AI分诊]
        G[/api/register<br>挂号处理]
        H[/admin/api/*<br>管理后台]
    end

    subgraph AI与数据层["🧠 AI 与数据层"]
        I[通义千问<br>Qwen-Turbo]
        J[百度语音<br>STT / TTS]
        K[SQLite 数据库<br>科室/医生/挂号记录]
    end

    A --> API
    B --> API
    C --> API
    D --> API
    API --> E
    API --> F
    API --> G
    API --> H
    E --> I
    F --> I
    G --> K
    H --> K

---

## 🛠️ 技术栈

### 桌面终端
| 技术 | 用途 |
|------|------|
| **Qt 5/6 (C++17)** | 跨平台 GUI 框架 |
| **QPainter / QtCharts** | 波形绘制与趋势图 |
| **QNetworkAccessManager** | HTTP 异步请求 |
| **QSS** | 动态主题切换（深色/浅色） |
| **CMake** | 构建系统 |

### 后端服务
| 技术 | 用途 |
|------|------|
| **Python 3.8+ / Flask** | RESTful API 服务 |
| **Flask-SQLAlchemy** | ORM 数据库操作 |
| **Flask-Login + bcrypt** | 管理员认证与密码加密 |
| **SQLite** | 数据持久化存储 |

### AI 与语音
| 服务 | 用途 |
|------|------|
| **通义千问 (Qwen-Turbo)** | 健康咨询、分诊建议 |
| **百度语音 (STT/TTS)** | 语音识别与合成 |

---

## 🚀 快速开始

### 环境要求
- **Linux**（Ubuntu 20.04+ 推荐）/ 树莓派（预留硬件接口）
- Qt 5.15+ 开发环境
- Python 3.8+
- CMake 3.10+

### 1️⃣ 克隆项目
```bash
git clone git@github.com:MonaJoshua/medical_agent_project.git
cd medical_agent_project

2️⃣ 配置环境变量（建议写入 ~/.bashrc 以永久生效。）
export QWEN_API_KEY="你的通义千问 API Key"
export BAIDU_API_KEY="你的百度语音 API Key"
export BAIDU_SECRET_KEY="你的百度语音 Secret Key"

3️⃣ 启动项目
chmod +x start.sh
./start.sh

启动脚本会提供以下选项：
1) 只启动后端 (Flask 服务，端口 5000)
2) 只启动前端 (Qt 终端)
3) 同时启动后端 + 前端
4) 退出

4️⃣ 访问管理后台
浏览器打开：
http://127.0.0.1:5000/admin
或：http://192.168.109.128:5000/admin

用户名：admin
密码：admin123

📁 项目结构
medical_agent_project/
├── start.sh                      # 统一启动脚本
├── backend/                      # Flask 后端
│   ├── backend_server.py         # 主服务入口
│   ├── admin/                    # Web 管理后台
│   │   ├── index.html
│   │   ├── css/style.css
│   │   └── js/admin.js
│   ├── instance/
│   │   └── medical.db            # SQLite 数据库
│   └── requirements.txt
└── medical_robot/                # Qt 桌面终端
    ├── CMakeLists.txt
    ├── config.yaml               # 配置文件
    ├── main.cpp
    ├── core/                     # 核心模块
    │   ├── config.cpp/h
    │   ├── datamanager.cpp/h
    │   ├── datastorage.cpp/h
    │   └── hardware_sensor.cpp/h # 树莓派硬件接口预留
    ├── network/                  # 网络模块
    │   ├── apiclient.cpp/h
    │   └── voicemanager.cpp/h
    ├── resources/                # QSS 资源
    │   ├── light.qss
    │   └── dark.qss
    └── ui/                       # 界面组件
        ├── mainwindow.cpp/h
        ├── voicechatwidget.cpp/h
        ├── smarttriage.cpp/h
        ├── patientmonitor.cpp/h
        ├── wavewidget.cpp/h
        ├── monitordatawidget.cpp/h
        ├── trendwidget.cpp/h
        ├── registrationrecordswidget.cpp/h
        └── moretoolswidget.cpp/h

🔧 后续开发计划
1.树莓派硬件接入：	取消注释 hardware_sensor.cpp 中的 USE_REAL_SENSORS 宏，实现真实传感器读取
2.开发更多功能：    等项目稳定后逐渐引入更多功能，如紧急联系人功能
3.Docker部署：     提供一键 Docker Compose 方案


[开发环境准备]
#开发环境说明： 
本项目在 Ubuntu 22.04（Linux 虚拟机） 中开发并测试通过。由于 Qt 和 Python 具有良好的跨平台特性，项目也支持在原生 Linux 系统或树莓派上运行。

## 1. 安装 Qt 开发环境及依赖
sudo apt update
sudo apt-get install qtbase5-dev qt5-qmake qtbase5-dev-tools \
    libqt5charts5-dev libqt5sql5-sqlite \
    libqt5network5 libqt5multimedia5

## 2. 安装 Python 虚拟环境及后端依赖
cd backend
python3 -m venv venv
source venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

📄 许可证
本项目采用 MIT License 开源协议。

🙏 致谢
通义千问 提供大模型 API
百度语音 提供 STT/TTS 服务
Qt 提供跨平台 GUI 框架

📧 联系方式
如有问题或建议，欢迎提 Issue 或联系项目维护者。