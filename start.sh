#!/bin/bash
# ============================================================
# 智能医疗助手 - 启动脚本
# ============================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目根目录
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BACKEND_DIR="$PROJECT_DIR/backend"
FRONTEND_DIR="$PROJECT_DIR/medical_robot"

echo -e "${BLUE}============================================================${NC}"
echo -e "${BLUE}         智能医疗助手 - 启动器${NC}"
echo -e "${BLUE}============================================================${NC}"

# ============================================================
# 检查环境变量
# ============================================================
echo -e "\n${YELLOW}[1/4] 检查环境变量...${NC}"

MISSING_VARS=()

# 检查通义千问 API Key
if [ -z "$QWEN_API_KEY" ]; then
    echo -e "  ${RED}❌ QWEN_API_KEY 未设置${NC}"
    MISSING_VARS+=("QWEN_API_KEY")
else
    echo -e "  ${GREEN}✅ QWEN_API_KEY 已设置${NC}"
fi

# 检查百度语音 API Key
if [ -z "$BAIDU_API_KEY" ]; then
    echo -e "  ${RED}❌ BAIDU_API_KEY 未设置${NC}"
    MISSING_VARS+=("BAIDU_API_KEY")
else
    echo -e "  ${GREEN}✅ BAIDU_API_KEY 已设置${NC}"
fi

# 检查百度语音 Secret Key
if [ -z "$BAIDU_SECRET_KEY" ]; then
    echo -e "  ${RED}❌ BAIDU_SECRET_KEY 未设置${NC}"
    MISSING_VARS+=("BAIDU_SECRET_KEY")
else
    echo -e "  ${GREEN}✅ BAIDU_SECRET_KEY 已设置${NC}"
fi

# 如果缺少环境变量，提示用户设置
if [ ${#MISSING_VARS[@]} -ne 0 ]; then
    echo -e "\n${YELLOW}⚠️ 以下环境变量未设置：${NC}"
    for var in "${MISSING_VARS[@]}"; do
        echo -e "  - $var"
    done
    echo -e "\n${YELLOW}请在终端执行以下命令设置环境变量（推荐写入 ~/.bashrc）：${NC}"
    echo -e "  ${BLUE}export QWEN_API_KEY=\"你的通义千问API Key\"${NC}"
    echo -e "  ${BLUE}export BAIDU_API_KEY=\"你的百度语音API Key\"${NC}"
    echo -e "  ${BLUE}export BAIDU_SECRET_KEY=\"你的百度语音Secret Key\"${NC}"
    echo ""
    read -p "是否继续启动？(y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${RED}已取消启动${NC}"
        exit 1
    fi
fi

# ============================================================
# 检查后端依赖
# ============================================================
echo -e "\n${YELLOW}[2/4] 检查后端依赖...${NC}"

if [ -d "$BACKEND_DIR/venv" ]; then
    echo -e "  ${GREEN}✅ Python 虚拟环境存在${NC}"
    source "$BACKEND_DIR/venv/bin/activate"
else
    echo -e "  ${YELLOW}⚠️ 未找到虚拟环境，正在创建...${NC}"
    cd "$BACKEND_DIR"
    python3 -m venv venv
    source venv/bin/activate
    pip install -r requirements.txt
    echo -e "  ${GREEN}✅ 依赖安装完成${NC}"
fi

# ============================================================
# 检查前端可执行文件
# ============================================================
echo -e "\n${YELLOW}[3/4] 检查前端可执行文件...${NC}"

if [ -f "$FRONTEND_DIR/build/MedicalRobot" ]; then
    echo -e "  ${GREEN}✅ 前端可执行文件存在${NC}"
else
    echo -e "  ${RED}❌ 未找到前端可执行文件，正在编译...${NC}"
    cd "$FRONTEND_DIR"
    mkdir -p build && cd build
    cmake .. && make -j$(nproc)
    echo -e "  ${GREEN}✅ 编译完成${NC}"
fi

# ============================================================
# 启动菜单
# ============================================================
echo -e "\n${YELLOW}[4/4] 选择启动方式${NC}"
echo -e "${BLUE}============================================================${NC}"
echo -e "  ${GREEN}1)${NC} 只启动后端 (Flask 服务，端口 5000)"
echo -e "  ${GREEN}2)${NC} 只启动前端 (Qt 终端)"
echo -e "  ${GREEN}3)${NC} 同时启动后端 + 前端"
echo -e "  ${GREEN}4)${NC} 退出"
echo -e "${BLUE}============================================================${NC}"
read -p "请选择 [1-4]: " choice

case $choice in
    1)
        echo -e "\n${GREEN}启动后端服务...${NC}"
        cd "$BACKEND_DIR"
        source venv/bin/activate
        python3 backend_server.py
        ;;
    2)
        echo -e "\n${GREEN}启动前端...${NC}"
        cd "$FRONTEND_DIR/build"
        ./MedicalRobot
        ;;
    3)
        echo -e "\n${GREEN}同时启动后端和前端...${NC}"
        cd "$BACKEND_DIR"
        source venv/bin/activate
        python3 backend_server.py &
        BACKEND_PID=$!
        echo -e "  ${BLUE}后端 PID: $BACKEND_PID${NC}"
        sleep 2
        cd "$FRONTEND_DIR/build"
        ./MedicalRobot
        # 前端退出后，关闭后端
        echo -e "\n${YELLOW}前端已退出，正在关闭后端...${NC}"
        kill $BACKEND_PID 2>/dev/null
        ;;
    4)
        echo -e "\n${GREEN}退出${NC}"
        exit 0
        ;;
    *)
        echo -e "\n${RED}无效选项，退出${NC}"
        exit 1
        ;;
esac
