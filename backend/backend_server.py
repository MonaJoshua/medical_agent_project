#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
import requests
import os
from datetime import datetime

app = Flask(__name__)
CORS(app)

# ========== 医院数据 ==========
# 初始默认配置
DEFAULT_HOSPITAL_DATA = {
    "内科": {"total": 10, "left": 3},
    "外科": {"total": 10, "left": 5},
    "神经内科": {"total": 8, "left": 2},
    "骨科": {"total": 10, "left": 6},
    "儿科": {"total": 12, "left": 4},
    "皮肤科": {"total": 5, "left": 1},
}

HOSPITAL_DATA = DEFAULT_HOSPITAL_DATA.copy()
registration_records = []

# ========== AI 配置 ==========
API_URL = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions"
API_KEY = os.environ.get("QWEN_API_KEY", "")
if not API_KEY:
    print("⚠️ 警告: 未设置环境变量 QWEN_API_KEY")
    print("请执行: export QWEN_API_KEY='你的API Key'")

# ========== 医院核心函数 ==========
def get_departments():
    return list(HOSPITAL_DATA.keys())

def get_slot(dept):
    if dept in HOSPITAL_DATA:
        return HOSPITAL_DATA[dept]["left"]
    return 0

def do_register(dept, patient_name, symptoms):
    if dept not in HOSPITAL_DATA:
        return {"success": False, "message": "科室不存在"}
    if HOSPITAL_DATA[dept]["left"] <= 0:
        return {"success": False, "message": "号源已满"}
    
    HOSPITAL_DATA[dept]["left"] -= 1
    ticket_id = f"{dept[:2]}{len(registration_records) + 1:04d}"
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    record = {
        "ticket_id": ticket_id,
        "department": dept,
        "patient_name": patient_name,
        "symptoms": symptoms,
        "remaining_slots": HOSPITAL_DATA[dept]["left"],
        "timestamp": timestamp
    }
    registration_records.append(record)
    
    return {
        "success": True,
        "message": f"挂号成功！{dept}剩余号源：{HOSPITAL_DATA[dept]['left']}",
        "ticket_id": ticket_id,
        "department": dept,
        "remaining_slots": HOSPITAL_DATA[dept]["left"],
        "queue_number": len(registration_records),
        "timestamp": timestamp
    }

# ========== AI 分诊函数 ==========
def ai_triage(symptoms):
    if not API_KEY:
        return "建议挂全科门诊（请配置 API Key）"
    
    headers = {
        "Authorization": f"Bearer {API_KEY}",
        "Content-Type": "application/json"
    }
    data = {
        "model": "qwen-turbo",
        "messages": [
            {"role": "user", "content": f"根据以下症状给出可能的科室分诊建议，只返回科室名称和建议：{symptoms}"}
        ],
        "max_tokens": 200
    }
    try:
        response = requests.post(API_URL, headers=headers, json=data, timeout=30)
        if response.status_code == 200:
            result = response.json()
            return result['choices'][0]['message']['content']
        else:
            return "建议挂全科门诊"
    except Exception as e:
        print(f"AI 分诊异常: {e}")
        return "建议挂全科门诊"

# ========== 管理后台页面 ==========
@app.route('/admin')
def admin_panel():
    """管理后台页面"""
    return send_from_directory('.', 'admin_panel.html')

# ========== API 路由 ==========
@app.route('/api/health', methods=['GET'])
def health():
    return jsonify({"status": "ok", "service": "智能医疗助手"})

@app.route('/api/departments', methods=['GET'])
def api_get_departments():
    return jsonify({"success": True, "departments": get_departments()})

@app.route('/api/slot/<dept>', methods=['GET'])
def api_get_slot(dept):
    return jsonify({"success": True, "department": dept, "remaining_slots": get_slot(dept)})

@app.route('/api/triage', methods=['POST'])
def api_triage():
    data = request.get_json()
    symptoms = data.get('symptoms', '')
    if not symptoms:
        return jsonify({"success": False, "message": "请提供症状描述"})
    result = ai_triage(symptoms)
    return jsonify({"success": True, "result": result})

@app.route('/api/register', methods=['POST'])
def api_register():
    data = request.get_json()
    department = data.get('department', '')
    patient_name = data.get('patient_name', '未知患者')
    symptoms = data.get('symptoms', '')
    
    if not department:
        return jsonify({"success": False, "message": "请选择科室"})
    
    result = do_register(department, patient_name, symptoms)
    return jsonify(result)

@app.route('/api/hospital/status', methods=['GET'])
def api_hospital_status():
    status = {}
    for dept, data in HOSPITAL_DATA.items():
        status[dept] = {
            "total": data["total"],
            "left": data["left"],
            "used": data["total"] - data["left"]
        }
    return jsonify({
        "success": True,
        "hospital_status": status,
        "total_registrations": len(registration_records)
    })

# ========== 管理后台 API ==========
@app.route('/api/hospital/reset', methods=['POST'])
def api_reset_hospital():
    """重置所有号源到初始值"""
    global HOSPITAL_DATA
    HOSPITAL_DATA = DEFAULT_HOSPITAL_DATA.copy()
    print("✅ 号源已重置")
    return jsonify({"success": True, "message": "号源已重置"})

@app.route('/api/hospital/department', methods=['POST'])
def api_add_department():
    """添加新科室"""
    data = request.get_json()
    name = data.get('name', '').strip()
    total = data.get('total', 10)
    left = data.get('left', total)
    
    if not name:
        return jsonify({"success": False, "message": "科室名称不能为空"})
    
    if name in HOSPITAL_DATA:
        return jsonify({"success": False, "message": "科室已存在"})
    
    HOSPITAL_DATA[name] = {"total": total, "left": left}
    print(f"✅ 添加科室: {name} (总:{total}, 剩:{left})")
    return jsonify({"success": True, "message": f"科室 {name} 已添加"})

@app.route('/api/hospital/department/<name>', methods=['PUT'])
def api_update_department(name):
    """更新科室号源"""
    data = request.get_json()
    total = data.get('total')
    left = data.get('left')
    
    if name not in HOSPITAL_DATA:
        return jsonify({"success": False, "message": "科室不存在"})
    
    if total is not None:
        HOSPITAL_DATA[name]["total"] = total
    if left is not None:
        HOSPITAL_DATA[name]["left"] = left
    
    print(f"✅ 更新科室: {name} -> 总:{HOSPITAL_DATA[name]['total']}, 剩:{HOSPITAL_DATA[name]['left']}")
    return jsonify({"success": True, "message": f"科室 {name} 已更新"})

@app.route('/api/hospital/department/<name>', methods=['DELETE'])
def api_delete_department(name):
    """删除科室"""
    if name not in HOSPITAL_DATA:
        return jsonify({"success": False, "message": "科室不存在"})
    
    # 不允许删除有挂号记录的科室（可选）
    # 直接删除
    del HOSPITAL_DATA[name]
    print(f"✅ 删除科室: {name}")
    return jsonify({"success": True, "message": f"科室 {name} 已删除"})

@app.route('/api/hospital/records', methods=['GET'])
def api_get_records():
    """获取挂号记录"""
    limit = request.args.get('limit', 50, type=int)
    records = registration_records[-limit:] if limit > 0 else registration_records
    return jsonify({
        "success": True,
        "records": records[::-1],  # 倒序，最新的在前
        "total": len(registration_records)
    })

@app.route('/api/hospital/records', methods=['DELETE'])
def api_clear_records():
    """清空挂号记录"""
    global registration_records
    registration_records = []
    print("✅ 挂号记录已清空")
    return jsonify({"success": True, "message": "挂号记录已清空"})

# ========== 启动服务 ==========
if __name__ == '__main__':
    print("=" * 60)
    print("🏥 智能医疗助手 - 后端服务")
    print("=" * 60)
    print("📍 API地址: http://127.0.0.1:5000")
    print("📋 接口列表:")
    print("   GET  /api/health           - 健康检查")
    print("   POST /api/triage           - AI分诊")
    print("   GET  /api/departments      - 科室列表")
    print("   POST /api/register         - 挂号")
    print("   GET  /api/hospital/status  - 医院状态")
    print("=" * 60)
    print("📋 管理后台接口:")
    print("   POST /api/hospital/reset           - 重置号源")
    print("   POST /api/hospital/department      - 添加科室")
    print("   PUT  /api/hospital/department/<名> - 更新科室")
    print("   DELETE /api/hospital/department/<名> - 删除科室")
    print("   GET  /api/hospital/records         - 挂号记录")
    print("   DELETE /api/hospital/records       - 清空记录")
    print("=" * 60)
    print("🌐 管理后台: http://127.0.0.1:5000/admin")
    print("=" * 60)
    print("当前号源:")
    for dept, data in HOSPITAL_DATA.items():
        print(f"   {dept}: {data['left']}/{data['total']}")
    print("=" * 60)
    
    app.run(host='0.0.0.0', port=5000, debug=True)