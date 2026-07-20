#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
智能医疗助手后端 - 支持桌面终端 + 管理后台
整合 SQLite 数据库，支持科室/医生/号源管理等
"""

import os
import hashlib
import requests
import bcrypt
from datetime import datetime, date, timedelta
from flask import Flask, request, jsonify, render_template, send_from_directory
from flask_cors import CORS
from flask_login import LoginManager, login_user, logout_user, login_required, current_user, UserMixin
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
from dotenv import load_dotenv
load_dotenv()

app = Flask(__name__, static_folder='admin', static_url_path='/admin/static')
app.config['SECRET_KEY'] = 'your-secret-key-change-in-production'
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///medical.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
CORS(app)

db = SQLAlchemy(app)
login_manager = LoginManager(app)
login_manager.login_view = '/admin/login'

# ========== 数据库模型 ==========
class User(db.Model, UserMixin):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    password_hash = db.Column(db.String(128), nullable=False)  # 存储 bcrypt 哈希

    def set_password(self, password):
        self.password_hash = bcrypt.hashpw(password.encode('utf-8'), bcrypt.gensalt()).decode('utf-8')

    def check_password(self, password):
        return bcrypt.checkpw(password.encode('utf-8'), self.password_hash.encode('utf-8'))

class Department(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), unique=True, nullable=False)
    description = db.Column(db.String(200))
    is_active = db.Column(db.Boolean, default=True)

class Doctor(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    department_id = db.Column(db.Integer, db.ForeignKey('department.id'))
    title = db.Column(db.String(100))
    specialty = db.Column(db.String(200))
    department = db.relationship('Department', backref='doctors')

class DoctorSchedule(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    doctor_id = db.Column(db.Integer, db.ForeignKey('doctor.id'))
    date = db.Column(db.Date, nullable=False)
    total_slots = db.Column(db.Integer, default=0)
    remaining_slots = db.Column(db.Integer, default=0)
    start_time = db.Column(db.String(10), default="08:00")
    end_time = db.Column(db.String(10), default="17:00")
    doctor = db.relationship('Doctor', backref='schedules')

class Registration(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    ticket_id = db.Column(db.String(20), unique=True)
    patient_name = db.Column(db.String(100))
    department_name = db.Column(db.String(100))
    doctor_name = db.Column(db.String(100))
    symptoms = db.Column(db.String(500))
    appointment_date = db.Column(db.Date)
    time_slot = db.Column(db.String(20))
    status = db.Column(db.String(20), default="waiting")
    created_at = db.Column(db.DateTime, default=datetime.now)

class OperationLog(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer)
    action = db.Column(db.String(50), nullable=False)
    target = db.Column(db.String(100))
    details = db.Column(db.String(500))
    timestamp = db.Column(db.DateTime, default=datetime.now)

class ApiClient(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)          # 终端名称/医院名称
    api_key = db.Column(db.String(64), unique=True, nullable=False)
    is_active = db.Column(db.Boolean, default=True)
    created_at = db.Column(db.DateTime, default=datetime.now)
    last_used = db.Column(db.DateTime)

    def generate_key(self):
        import secrets
        self.api_key = secrets.token_urlsafe(32)  # 生成强随机 Key

# ========== AI 配置 ==========
QWEN_API_KEY = os.environ.get("QWEN_API_KEY", "")
if not QWEN_API_KEY:
    print("⚠️ 警告: 未设置环境变量 QWEN_API_KEY")
    print("请执行: export QWEN_API_KEY='你的Key'")

API_URL = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions"

def ai_triage(symptoms):
    if not QWEN_API_KEY:
        return "建议挂全科门诊（未配置API Key）"
    headers = {"Authorization": f"Bearer {QWEN_API_KEY}", "Content-Type": "application/json"}
    data = {
        "model": "qwen-turbo",
        "messages": [{"role": "user", "content": f"根据以下症状给出可能的科室分诊建议：{symptoms}"}],
        "max_tokens": 200
    }
    try:
        resp = requests.post(API_URL, headers=headers, json=data, timeout=30)
        if resp.status_code == 200:
            return resp.json()['choices'][0]['message']['content']
        return "建议挂全科门诊"
    except Exception as e:
        print(f"AI 分诊异常: {e}")
        return "建议挂全科门诊"

# ========== API Key 验证 ==========
def verify_api_key(request):
    key = request.headers.get('X-API-Key')
    if not key:
        return False
    client = ApiClient.query.filter_by(api_key=key, is_active=True).first()
    if client:
        client.last_used = datetime.now()
        db.session.commit()
        return True
    return False

# ========== 操作日志辅助函数 ==========
def log_operation(action, target="", details=""):
    with app.app_context():
        try:
            user_id = current_user.id if current_user and current_user.is_authenticated else 0
            log = OperationLog(user_id=user_id, action=action, target=target, details=details)
            db.session.add(log)
            count = OperationLog.query.count()
            if count > 200:
                oldest = OperationLog.query.order_by(OperationLog.timestamp.asc()).limit(count - 200).all()
                for o in oldest:
                    db.session.delete(o)
            db.session.commit()
        except Exception as e:
            print("记录操作日志异常:", e)
            db.session.rollback()

# ========== 初始化数据库 ==========
def init_db():
    with app.app_context():
        db.create_all()
        if not User.query.filter_by(username='admin').first():
            admin = User(username='admin')
            admin.set_password('admin123')  # 使用 bcrypt 哈希
            db.session.add(admin)
            db.session.commit()
            print("✅ 创建管理员账户: admin / admin123")
        if Department.query.count() == 0:
            default_depts = ["全科门诊","内科", "外科",  "儿科", "皮肤科"]
            for name in default_depts:
                db.session.add(Department(name=name, is_active=True))
            db.session.commit()
            print("✅ 导入默认科室")
        if Doctor.query.count() == 0:
            dept = Department.query.filter_by(name="内科").first()
            if dept:
                doc = Doctor(name="李医生", department_id=dept.id, title="主任医师", specialty="常见病诊疗")
                db.session.add(doc)
                db.session.commit()
                today = date.today()
                if not DoctorSchedule.query.filter_by(doctor_id=doc.id, date=today).first():
                    sched = DoctorSchedule(doctor_id=doc.id, date=today, total_slots=20, remaining_slots=20)
                    db.session.add(sched)
            db.session.commit()
            print("✅ 导入示例医生和号源")

        # 在 init_db() 函数中添加,后续可删除
        if ApiClient.query.count() == 0:
            client = ApiClient(name="默认终端")
            client.generate_key()
            db.session.add(client)
            db.session.commit()
            print(f"✅ 创建默认 API Key: {client.api_key}")

init_db()

@login_manager.user_loader
def load_user(user_id):
    return User.query.get(int(user_id))

# ========== 桌面终端 API ==========
@app.route('/api/health', methods=['GET'])
def health():
    return jsonify({"status": "ok", "service": "智能医疗助手"})

@app.route('/api/departments', methods=['GET'])
def get_departments():
    depts = Department.query.filter_by(is_active=True).all()
    return jsonify({"success": True, "departments": [d.name for d in depts]})

@app.route('/api/slot/<dept_name>', methods=['GET'])
def get_slot(dept_name):
    dept = Department.query.filter_by(name=dept_name).first()
    if not dept:
        return jsonify({"success": False, "message": "科室不存在"})
    today = date.today()
    total_remaining = 0
    for doc in dept.doctors:
        schedule = DoctorSchedule.query.filter_by(doctor_id=doc.id, date=today).first()
        if schedule:
            total_remaining += schedule.remaining_slots
    return jsonify({"success": True, "department": dept_name, "remaining_slots": total_remaining})

@app.route('/api/register', methods=['POST'])
def register():
    data = request.get_json()
    patient_name = data.get('patient_name', '未知患者')
    department_name = data.get('department', '')
    symptoms = data.get('symptoms', '')
    
    dept = Department.query.filter_by(name=department_name).first()
    if not dept:
        return jsonify({"success": False, "message": "科室不存在"})
    
    today = date.today()
    
    # ===== 防重校验 =====
    existing = Registration.query.filter_by(
        patient_name=patient_name,
        department_name=department_name,
        appointment_date=today,
        status='waiting'
    ).first()
    if existing:
        return jsonify({
            "success": False,
            "message": f"您今天已在 {department_name} 挂号，请勿重复挂号"
        })
    
    for doc in dept.doctors:
        schedule = DoctorSchedule.query.filter_by(doctor_id=doc.id, date=today).first()
        if schedule and schedule.remaining_slots > 0:
            schedule.remaining_slots -= 1
            ticket_id = f"{department_name[:2]}{Registration.query.count()+1:04d}"
            reg = Registration(
                ticket_id=ticket_id,
                patient_name=patient_name,
                department_name=department_name,
                doctor_name=doc.name,
                symptoms=symptoms,
                appointment_date=today,
                status="waiting"
            )
            db.session.add(reg)
            db.session.commit()
            return jsonify({
                "success": True,
                "message": "挂号成功",
                "ticket_id": ticket_id,
                "department": department_name,
                "doctor": doc.name,
                "queue_number": Registration.query.count(),
                "remaining_slots": schedule.remaining_slots
            })
    return jsonify({"success": False, "message": "当前无号源"})

@app.route('/api/triage', methods=['POST'])
def api_triage():
    data = request.get_json()
    symptoms = data.get('symptoms', '')
    if not symptoms:
        return jsonify({"success": False, "message": "请提供症状描述"})
    result = ai_triage(symptoms)
    return jsonify({"success": True, "result": result})

@app.route('/api/hospital/status', methods=['GET'])
def hospital_status():
    total_reg = Registration.query.count()
    return jsonify({"success": True, "total_registrations": total_reg})

# ========== 管理后台 API（需登录）==========
@app.route('/admin/login', methods=['POST'])
def admin_login():
    data = request.get_json()
    username = data.get('username')
    password = data.get('password')
    user = User.query.filter_by(username=username).first()
    if user and user.check_password(password):
        login_user(user)
        log_operation("login", "管理员登录", f"用户 {username} 登录成功")
        return jsonify({"success": True, "message": "登录成功"})
    return jsonify({"success": False, "message": "用户名或密码错误"}), 401

@app.route('/admin/logout', methods=['POST'])
@login_required
def admin_logout():
    logout_user()
    log_operation("logout", "管理员退出", "用户退出登录")
    return jsonify({"success": True, "message": "已退出"})

@app.route('/admin/api/departments', methods=['GET', 'POST', 'PUT', 'DELETE'])
@login_required
def manage_departments():
    if request.method == 'GET':
        depts = Department.query.all()
        return jsonify({"success": True, "data": [{"id": d.id, "name": d.name, "description": d.description, "is_active": d.is_active} for d in depts]})
    elif request.method == 'POST':
        data = request.json
        dept = Department(name=data['name'], description=data.get('description', ''), is_active=data.get('is_active', True))
        db.session.add(dept)
        db.session.commit()
        log_operation("add_dept", "科室管理", f"新增科室: {data['name']}")
        return jsonify({"success": True, "id": dept.id})
    elif request.method == 'PUT':
        data = request.json
        dept = Department.query.get(data['id'])
        if not dept:
            return jsonify({"success": False, "message": "科室不存在"}), 404
        old_name = dept.name
        dept.name = data.get('name', dept.name)
        dept.description = data.get('description', dept.description)
        dept.is_active = data.get('is_active', dept.is_active)
        db.session.commit()
        log_operation("edit_dept", "科室管理", f"编辑科室 ID:{data['id']}, 旧名称:{old_name}, 新名称:{dept.name}")
        return jsonify({"success": True})
    elif request.method == 'DELETE':
        dept_id = request.args.get('id')
        dept = Department.query.get(dept_id)
        if dept:
            dept_name = dept.name
            db.session.delete(dept)
            db.session.commit()
            log_operation("del_dept", "科室管理", f"删除科室 ID:{dept_id}, 名称:{dept_name}")
        return jsonify({"success": True})

@app.route('/admin/api/doctors', methods=['GET', 'POST', 'PUT', 'DELETE'])
@login_required
def manage_doctors():
    if request.method == 'GET':
        doctors = Doctor.query.all()
        data = [{
            'id': d.id,
            'name': d.name,
            'department_id': d.department_id,
            'department_name': d.department.name if d.department else '',
            'title': d.title or '',
            'specialty': d.specialty or ''
        } for d in doctors]
        return jsonify({'success': True, 'data': data})
    elif request.method == 'POST':
        data = request.json
        doc = Doctor(
            name=data['name'],
            department_id=data.get('department_id'),
            title=data.get('title', ''),
            specialty=data.get('specialty', '')
        )
        db.session.add(doc)
        db.session.commit()
        log_operation("add_doctor", "医生管理", f"新增医生: {data['name']}")
        return jsonify({'success': True, 'id': doc.id})
    elif request.method == 'PUT':
        data = request.json
        doc = Doctor.query.get(data['id'])
        if not doc:
            return jsonify({'success': False, 'message': '医生不存在'}), 404
        old_name = doc.name
        doc.name = data.get('name', doc.name)
        doc.department_id = data.get('department_id', doc.department_id)
        doc.title = data.get('title', doc.title)
        doc.specialty = data.get('specialty', doc.specialty)
        db.session.commit()
        log_operation("edit_doctor", "医生管理", f"编辑医生 ID:{data['id']}, 旧姓名:{old_name}, 新姓名:{doc.name}")
        return jsonify({'success': True})
    elif request.method == 'DELETE':
        doc_id = request.args.get('id')
        doc = Doctor.query.get(doc_id)
        if doc:
            doc_name = doc.name
            db.session.delete(doc)
            db.session.commit()
            log_operation("del_doctor", "医生管理", f"删除医生 ID:{doc_id}, 姓名:{doc_name}")
        return jsonify({'success': True})

# ========== 号源管理 API ==========
@app.route('/admin/api/schedules', methods=['GET', 'POST', 'PUT', 'DELETE'])
@login_required
def manage_schedules():
    """管理医生号源"""
    if request.method == 'GET':
        # 获取所有号源（含医生和科室信息）
        schedules = db.session.query(
            DoctorSchedule,
            Doctor.name.label('doctor_name'),
            Department.name.label('department_name')
        ).join(Doctor, DoctorSchedule.doctor_id == Doctor.id)\
         .join(Department, Doctor.department_id == Department.id)\
         .order_by(DoctorSchedule.date.desc()).all()
        
        result = []
        for sched, doc_name, dept_name in schedules:
            result.append({
                'id': sched.id,
                'doctor_id': sched.doctor_id,
                'doctor_name': doc_name,
                'department_name': dept_name,
                'date': sched.date.strftime('%Y-%m-%d'),
                'total_slots': sched.total_slots,
                'remaining_slots': sched.remaining_slots,
                'start_time': sched.start_time,
                'end_time': sched.end_time
            })
        return jsonify({"success": True, "data": result})
    
    elif request.method == 'POST':
        data = request.json
        doctor_id = data.get('doctor_id')
        date_str = data.get('date')
        total_slots = data.get('total_slots', 10)
        start_time = data.get('start_time', '08:00')
        end_time = data.get('end_time', '17:00')
        
        if not doctor_id or not date_str:
            return jsonify({"success": False, "message": "请选择医生和日期"})
        
        # 检查是否已存在该医生当天的号源
        from datetime import datetime
        date_obj = datetime.strptime(date_str, '%Y-%m-%d').date()
        existing = DoctorSchedule.query.filter_by(doctor_id=doctor_id, date=date_obj).first()
        if existing:
            return jsonify({"success": False, "message": "该医生当天已有号源配置"})
        
        schedule = DoctorSchedule(
            doctor_id=doctor_id,
            date=date_obj,
            total_slots=total_slots,
            remaining_slots=total_slots,
            start_time=start_time,
            end_time=end_time
        )
        db.session.add(schedule)
        db.session.commit()
        log_operation("add_schedule", "号源管理", f"新增号源 医生ID:{doctor_id}, 日期:{date_str}, 号源数:{total_slots}")
        return jsonify({"success": True, "message": "号源配置成功", "id": schedule.id})
    
    elif request.method == 'PUT':
        data = request.json
        schedule_id = data.get('id')
        if not schedule_id:
            return jsonify({"success": False, "message": "缺少号源ID"})
        
        schedule = DoctorSchedule.query.get(schedule_id)
        if not schedule:
            return jsonify({"success": False, "message": "号源不存在"})
        
        # 更新字段
        if 'total_slots' in data:
            total = data['total_slots']
            remaining_change = total - schedule.total_slots
            schedule.total_slots = total
            schedule.remaining_slots += remaining_change
        if 'start_time' in data:
            schedule.start_time = data['start_time']
        if 'end_time' in data:
            schedule.end_time = data['end_time']
        
        db.session.commit()
        log_operation("edit_schedule", "号源管理", f"编辑号源 ID:{schedule_id}, 新总数:{data.get('total_slots', schedule.total_slots)}")
        return jsonify({"success": True, "message": "号源更新成功"})
    
    elif request.method == 'DELETE':
        schedule_id = request.args.get('id')
        if not schedule_id:
            return jsonify({"success": False, "message": "缺少号源ID"})
        
        schedule = DoctorSchedule.query.get(schedule_id)
        if schedule:
            db.session.delete(schedule)
            db.session.commit()
            log_operation("del_schedule", "号源管理", f"删除号源 ID:{schedule_id}")
            return jsonify({"success": True, "message": "号源已删除"})
        return jsonify({"success": False, "message": "号源不存在"})

# ========== 终端管理 API ==========
@app.route('/admin/api/clients', methods=['GET', 'POST', 'PUT', 'DELETE'])
@login_required
def manage_clients():
    if request.method == 'GET':
        clients = ApiClient.query.order_by(ApiClient.created_at.desc()).all()
        return jsonify({
            "success": True,
            "data": [{
                "id": c.id,
                "name": c.name,
                "api_key": c.api_key,
                "is_active": c.is_active,
                "created_at": c.created_at.strftime('%Y-%m-%d %H:%M:%S') if c.created_at else '',
                "last_used": c.last_used.strftime('%Y-%m-%d %H:%M:%S') if c.last_used else ''
            } for c in clients]
        })
    elif request.method == 'POST':
        data = request.json
        client = ApiClient(name=data['name'])
        client.generate_key()
        db.session.add(client)
        db.session.commit()
        return jsonify({"success": True, "id": client.id, "api_key": client.api_key})
    elif request.method == 'PUT':
        data = request.json
        client = ApiClient.query.get(data['id'])
        if not client:
            return jsonify({"success": False, "message": "终端不存在"}), 404
        if 'name' in data:
            client.name = data['name']
        if 'is_active' in data:
            client.is_active = data['is_active']
        db.session.commit()
        return jsonify({"success": True})
    elif request.method == 'DELETE':
        client_id = request.args.get('id')
        client = ApiClient.query.get(client_id)
        if client:
            db.session.delete(client)
            db.session.commit()
        return jsonify({"success": True})

# ========== 趋势图接口 ==========
@app.route('/admin/api/trend', methods=['GET'])
@login_required
def admin_trend():
    """获取近7天挂号趋势数据"""
    today = date.today()
    result = []
    for i in range(6, -1, -1):
        day = today - timedelta(days=i)
        count = Registration.query.filter_by(appointment_date=day).count()
        result.append({
            'date': day.strftime('%Y-%m-%d'),
            'day': f'{day.month}/{day.day}',
            'count': count
        })
    return jsonify({"success": True, "data": result})

# ========== 挂号记录 ==========
@app.route('/admin/api/registrations', methods=['GET'])
@login_required
def admin_get_registrations():
    dept = request.args.get('department')
    date_str = request.args.get('date')
    query = Registration.query
    if dept:
        query = query.filter_by(department_name=dept)
    if date_str:
        try:
            filter_date = datetime.strptime(date_str, '%Y-%m-%d').date()
            query = query.filter_by(appointment_date=filter_date)
        except:
            pass
    registrations = query.order_by(Registration.created_at.desc()).all()
    return jsonify({
        "success": True,
        "data": [{
            "id": r.id, 
            "ticket_id": r.ticket_id,
            "patient_name": r.patient_name,
            "department": r.department_name,
            "doctor": r.doctor_name,
            "date": str(r.appointment_date),
            "symptoms": r.symptoms,
            "status": r.status,
            "created_at": r.created_at.strftime('%Y-%m-%d %H:%M:%S') if r.created_at else ''
        } for r in registrations]
    })
    
@app.route('/admin/api/registrations/<int:reg_id>', methods=['PUT'])
@login_required
def admin_update_registration(reg_id):
    """更新挂号状态（已完成/已取消）"""
    reg = Registration.query.get(reg_id)
    if not reg:
        return jsonify({"success": False, "message": "挂号记录不存在"}), 404
    
    data = request.json
    new_status = data.get('status')
    
    if new_status not in ['waiting', 'completed', 'cancelled']:
        return jsonify({"success": False, "message": "无效的状态值"}), 400
    
    # 如果是从 waiting 改为 cancelled，释放号源
    if reg.status == 'waiting' and new_status == 'cancelled':
        # 查找并释放号源
        today = date.today()
        dept = Department.query.filter_by(name=reg.department_name).first()
        if dept:
            for doc in dept.doctors:
                if doc.name == reg.doctor_name:
                    schedule = DoctorSchedule.query.filter_by(doctor_id=doc.id, date=today).first()
                    if schedule:
                        schedule.remaining_slots += 1
                        break
    
    reg.status = new_status
    db.session.commit()
    
    log_operation("edit_registration", "挂号管理", f"更新挂号 ID:{reg_id}, 状态:{new_status}")
    
    return jsonify({"success": True, "message": "状态更新成功"})

# ========== 仪表盘统计 ==========
@app.route('/admin/api/stats', methods=['GET'])
@login_required
def admin_stats():
    today = date.today()
    today_regs = Registration.query.filter_by(appointment_date=today).count()
    total_regs = Registration.query.count()
    hot_depts = db.session.query(Registration.department_name, db.func.count()).group_by(Registration.department_name).order_by(db.func.count().desc()).limit(5).all()
    return jsonify({"success": True, "today_registrations": today_regs, "total_registrations": total_regs, "hot_departments": [{"name": d[0], "count": d[1]} for d in hot_depts]})

# ========== 操作日志查询 ==========
@app.route('/admin/api/logs', methods=['GET'])
@login_required
def admin_get_logs():
    '''获取操作日志列表，按时间倒序'''
    page = request.args.get('page', 1, type=int)
    per_page = request.args.get('per_page', 50, type=int)
    pagination = OperationLog.query.order_by(OperationLog.timestamp.desc()).paginate(
        page=page, per_page=per_page, error_out=False
    )
    logs = [{
        'id': log.id,
        'user_id': log.user_id,
        'action': log.action,
        'target': log.target,
        'details': log.details,
        'timestamp': log.timestamp.strftime('%Y-%m-%d %H:%M:%S')
    } for log in pagination.items]
    return jsonify({
        'success': True,
        'data': logs,
        'total': pagination.total,
        'page': pagination.page,
        'pages': pagination.pages
    })

# ========== 管理后台首页 ==========
@app.route('/admin')
def admin_index():
    return send_from_directory(app.static_folder, 'index.html')


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)