#!/usr/bin/env python3
"""
智能医疗助手后端 API 自动化测试
使用方法：
    python3 test_backend.py
    或
    pytest test_backend.py -v
"""

import requests
import json
import time
import sys
from typing import Dict, Any

BASE_URL = "http://127.0.0.1:5000"
TEST_PATIENT = "测试患者_自动化"

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'

def print_ok(msg: str):
    print(f"{Colors.GREEN}✅ {msg}{Colors.RESET}")

def print_fail(msg: str):
    print(f"{Colors.RED}❌ {msg}{Colors.RESET}")

def print_info(msg: str):
    print(f"{Colors.BLUE}📌 {msg}{Colors.RESET}")

def print_warn(msg: str):
    print(f"{Colors.YELLOW}⚠️ {msg}{Colors.RESET}")

def test_health() -> bool:
    """测试健康检查接口"""
    try:
        resp = requests.get(f"{BASE_URL}/api/health", timeout=5)
        assert resp.status_code == 200
        data = resp.json()
        assert data.get("status") == "ok"
        assert data.get("service") == "智能医疗助手"
        print_ok("健康检查接口 (GET /api/health)")
        return True
    except Exception as e:
        print_fail(f"健康检查接口失败: {e}")
        return False

def test_departments() -> bool:
    """测试科室列表接口"""
    try:
        resp = requests.get(f"{BASE_URL}/api/departments", timeout=5)
        assert resp.status_code == 200
        data = resp.json()
        assert data.get("success") == True
        departments = data.get("departments", [])
        assert len(departments) > 0
        assert "内科" in departments
        print_ok(f"科室列表接口 (GET /api/departments) - 共 {len(departments)} 个科室")
        return True
    except Exception as e:
        print_fail(f"科室列表接口失败: {e}")
        return False

def test_triage_normal() -> bool:
    """测试正常分诊 - 关键词匹配"""
    test_cases = [
        ("头疼头痛头晕", "神经内科"),
        ("咳嗽发烧", "呼吸内科"),
        ("胃痛腹痛", "消化内科"),
        ("牙痛", "口腔科"),
        ("眼睛视力模糊", "眼科"),
    ]
    
    all_pass = True
    for symptoms, expected_dept in test_cases:
        try:
            resp = requests.post(f"{BASE_URL}/api/triage", 
                                json={"symptoms": symptoms}, timeout=10)
            assert resp.status_code == 200
            data = resp.json()
            assert data.get("success") == True
            result = data.get("result", "")
            # 检查返回结果包含期望的科室名
            if expected_dept in result:
                print_ok(f"分诊接口 - '{symptoms[:10]}...' → {expected_dept}")
            else:
                print_warn(f"分诊接口 - '{symptoms[:10]}...' 期望包含 {expected_dept}，实际: {result[:50]}")
                all_pass = False
        except Exception as e:
            print_fail(f"分诊接口测试失败 '{symptoms}': {e}")
            all_pass = False
    
    return all_pass

def test_triage_empty() -> bool:
    """测试空症状分诊"""
    try:
        resp = requests.post(f"{BASE_URL}/api/triage", 
                            json={"symptoms": ""}, timeout=5)
        assert resp.status_code == 200
        data = resp.json()
        assert data.get("success") == False
        assert "请提供症状描述" in data.get("message", "")
        print_ok("分诊接口 - 空症状处理正确")
        return True
    except Exception as e:
        print_fail(f"空症状测试失败: {e}")
        return False

def test_register_normal() -> bool:
    """测试正常挂号"""
    try:
        # 第一次挂号
        resp1 = requests.post(f"{BASE_URL}/api/register",
                             json={
                                 "department": "内科",
                                 "patient_name": TEST_PATIENT,
                                 "symptoms": "头痛发烧"
                             }, timeout=10)
        assert resp1.status_code == 200
        data1 = resp1.json()
        assert data1.get("success") == True
        assert "ticket_id" in data1
        assert data1.get("department") == "内科"
        assert data1.get("queue_number") == 1
        print_ok(f"挂号接口 - 第一次挂号成功，号单: {data1.get('ticket_id')}")
        
        # 第二次挂号（测试排队号递增）
        resp2 = requests.post(f"{BASE_URL}/api/register",
                             json={
                                 "department": "外科",
                                 "patient_name": TEST_PATIENT,
                                 "symptoms": "手外伤"
                             }, timeout=10)
        data2 = resp2.json()
        assert data2.get("queue_number") == 2
        print_ok(f"挂号接口 - 第二次挂号成功，排队号: {data2.get('queue_number')}")
        
        return True
    except Exception as e:
        print_fail(f"挂号接口测试失败: {e}")
        return False

def test_register_invalid_dept() -> bool:
    """测试挂号 - 无效科室"""
    try:
        resp = requests.post(f"{BASE_URL}/api/register",
                             json={
                                 "department": "不存在科室",
                                 "patient_name": TEST_PATIENT,
                                 "symptoms": "测试"
                             }, timeout=5)
        assert resp.status_code == 200
        data = resp.json()
        assert data.get("success") == False
        assert "科室不存在" in data.get("message", "")
        print_ok("挂号接口 - 无效科室处理正确")
        return True
    except Exception as e:
        print_fail(f"无效科室测试失败: {e}")
        return False

def test_hospital_status() -> bool:
    """测试医院状态接口"""
    try:
        resp = requests.get(f"{BASE_URL}/api/hospital/status", timeout=5)
        assert resp.status_code == 200
        data = resp.json()
        assert data.get("success") == True
        status = data.get("hospital_status", {})
        assert "内科" in status
        assert "total" in status["内科"]
        assert "left" in status["内科"]
        print_ok(f"医院状态接口 - 当前总挂号人数: {data.get('total_registrations', 0)}")
        return True
    except Exception as e:
        print_fail(f"医院状态接口失败: {e}")
        return False

def test_slot_query() -> bool:
    """测试号源查询接口"""
    try:
        resp = requests.get(f"{BASE_URL}/api/slot/内科", timeout=5)
        assert resp.status_code == 200
        data = resp.json()
        assert data.get("success") == True
        assert data.get("department") == "内科"
        assert "remaining_slots" in data
        print_ok(f"号源查询接口 - 内科剩余号源: {data.get('remaining_slots')}")
        return True
    except Exception as e:
        print_fail(f"号源查询接口失败: {e}")
        return False

def test_register_duplicate() -> bool:
    """测试科室号源消耗（验证号源递减）"""
    try:
        # 查询当前号源
        resp_before = requests.get(f"{BASE_URL}/api/slot/皮肤科", timeout=5)
        slots_before = resp_before.json().get("remaining_slots", 0)
        
        # 挂号一次
        resp_reg = requests.post(f"{BASE_URL}/api/register",
                                 json={
                                     "department": "皮肤科",
                                     "patient_name": TEST_PATIENT,
                                     "symptoms": "皮疹"
                                 }, timeout=5)
        assert resp_reg.json().get("success") == True
        
        # 再次查询号源
        resp_after = requests.get(f"{BASE_URL}/api/slot/皮肤科", timeout=5)
        slots_after = resp_after.json().get("remaining_slots", 0)
        
        assert slots_after == slots_before - 1
        print_ok(f"号源递减测试 - {slots_before} → {slots_after}")
        return True
    except Exception as e:
        print_fail(f"号源递减测试失败: {e}")
        return False

def main():
    print("\n" + "="*60)
    print("🏥 智能医疗助手后端 API 自动化测试")
    print("="*60 + "\n")
    
    # 1. 检查后端服务是否运行
    print_info("检查后端服务连接...")
    try:
        requests.get(f"{BASE_URL}/api/health", timeout=3)
        print_ok("后端服务已连接")
    except:
        print_fail("后端服务未运行！请先启动: python3 backend_server.py")
        sys.exit(1)
    
    print("\n" + "-"*40)
    print("开始执行测试用例")
    print("-"*40 + "\n")
    
    tests = [
        ("健康检查", test_health),
        ("科室列表", test_departments),
        ("正常分诊（关键词匹配）", test_triage_normal),
        ("空症状处理", test_triage_empty),
        ("正常挂号", test_register_normal),
        ("无效科室挂号", test_register_invalid_dept),
        ("医院状态查询", test_hospital_status),
        ("号源查询", test_slot_query),
        ("号源递减验证", test_register_duplicate),
    ]
    
    results = []
    for name, test_func in tests:
        print_info(f"测试: {name}")
        result = test_func()
        results.append((name, result))
        print()
    
    # 统计结果
    print("="*60)
    print("测试结果汇总")
    print("="*60)
    
    passed = sum(1 for _, r in results if r)
    total = len(results)
    
    for name, result in results:
        status = f"{Colors.GREEN}通过{Colors.RESET}" if result else f"{Colors.RED}失败{Colors.RESET}"
        print(f"  {name}: {status}")
    
    print(f"\n总计: {passed}/{total} 通过")
    
    if passed == total:
        print(f"\n{Colors.GREEN}🎉 所有测试通过！{Colors.RESET}")
        return 0
    else:
        print(f"\n{Colors.RED}❌ 有 {total - passed} 个测试失败，请检查{Colors.RESET}")
        return 1

if __name__ == "__main__":
    sys.exit(main())
