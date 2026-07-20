#!/usr/bin/env python3
"""
并发挂号压力测试
模拟50个终端同时挂号，测试后端响应时间和稳定性
"""
import requests
import concurrent.futures
import time
import statistics
import sys

API_URL = "http://127.0.0.1:5000/api/register"
API_KEY = "KRNfZA8f3IkcfQze3rNv-WAdljUvtj02u5PdW1Y87OA"

# 测试数据（100个不同患者）
test_patients = [
    {"patient_name": f"压力测试患者{i}", "department": "内科", "symptoms": "头痛发烧"} 
    for i in range(100)
]

def register(patient_data):
    """执行单个挂号请求"""
    headers = {"X-API-Key": API_KEY, "Content-Type": "application/json"}
    try:
        start = time.time()
        resp = requests.post(API_URL, json=patient_data, headers=headers, timeout=10)
        elapsed = time.time() - start
        
        if resp.status_code == 200:
            data = resp.json()
            if data.get('success'):
                return {"success": True, "time": elapsed, "ticket": data.get('ticket_id')}
            else:
                return {"success": False, "error": data.get('message'), "time": elapsed}
        else:
            return {"success": False, "error": f"HTTP {resp.status_code}", "time": elapsed}
    except requests.exceptions.Timeout:
        return {"success": False, "error": "超时", "time": 5}
    except Exception as e:
        return {"success": False, "error": str(e), "time": 0}

def run_stress_test(concurrency=50):
    """运行压力测试"""
    print(f"🏥 开始压力测试，并发数: {concurrency}")
    print("=" * 50)
    
    # 取前 concurrency 个患者
    patients = test_patients[:concurrency]
    
    # 并发执行挂号请求
    start_time = time.time()
    with concurrent.futures.ThreadPoolExecutor(max_workers=concurrency) as executor:
        results = list(executor.map(register, patients))
    total_time = time.time() - start_time
    
    # 统计分析
    success_count = sum(1 for r in results if r['success'])
    fail_count = concurrency - success_count
    times = [r['time'] for r in results if r['success']]
    
    print(f"\n📊 测试结果:")
    print(f"  总请求数: {concurrency}")
    print(f"  成功: {success_count}")
    print(f"  失败: {fail_count}")
    print(f"  成功率: {(success_count/concurrency*100):.1f}%")
    print(f"  总耗时: {total_time:.2f}s")
    
    if times:
        print(f"\n📈 响应时间统计 (成功请求):")
        print(f"  平均响应时间: {statistics.mean(times):.3f}s")
        print(f"  最小响应时间: {min(times):.3f}s")
        print(f"  最大响应时间: {max(times):.3f}s")
        if len(times) > 1:
            print(f"  标准差: {statistics.stdev(times):.3f}s")
    else:
        print("\n❌ 所有请求均失败")
    
    print("=" * 50)
    return results

if __name__ == "__main__":
    # 检查后端是否可用
    try:
        resp = requests.get("http://127.0.0.1:5000/api/health", timeout=3)
        if resp.status_code != 200:
            print("❌ 后端服务未运行或不可用，请先启动后端")
            sys.exit(1)
        print("✅ 后端服务已连接")
    except Exception:
        print("❌ 后端服务未运行或不可用，请先启动后端")
        sys.exit(1)
    
    # 运行压力测试
    run_stress_test(50)
