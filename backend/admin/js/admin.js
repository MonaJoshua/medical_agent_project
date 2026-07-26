'use strict';

/* ===================================================================
   工具函数
   ===============================================================*/

// Toast 提示
function showToast(message, type) {
    const toastEl = document.getElementById('toastMsg');
    const toastBody = document.getElementById('toastBody');
    const toastTitle = document.getElementById('toastTitle');
    toastBody.textContent = message;
    toastTitle.textContent = type === 'error' ? '❌ 错误' : '✅ 成功';
    toastEl.className = 'toast';
    if (type === 'error') toastEl.classList.add('bg-danger', 'text-white');
    else toastEl.classList.add('bg-success', 'text-white');
    const toast = bootstrap.Toast.getOrCreateInstance(toastEl, { delay: 3000 });
    toast.show();
}

// 通用 API 请求
async function apiRequest(url, options) {
    try {
        const resp = await fetch(url, {
            credentials: 'include',
            headers: { 'Content-Type': 'application/json' },
            ...options
        });
        if (resp.status === 401) {
            showToast('登录已过期，请重新登录', 'error');
            getLoginModal().show();
            return null;
        }
        return await resp.json();
    } catch (err) {
        showToast('网络请求失败: ' + err.message, 'error');
        return null;
    }
}

// HTML 转义
function escapeHtml(str) {
    const div = document.createElement('div');
    div.textContent = str;
    return div.innerHTML;
}

/* ===================================================================
   登录 / 退出
   ===============================================================*/

let loginModal = null;

function getLoginModal() {
    if (!loginModal) {
        loginModal = new bootstrap.Modal(document.getElementById('loginModal'), {
            backdrop: 'static', keyboard: false
        });
    }
    return loginModal;
}

// Initialize login modal on DOMContentLoaded
document.addEventListener('DOMContentLoaded', function() {
    getLoginModal();
});

document.getElementById('loginBtn').addEventListener('click', async function () {
    const username = document.getElementById('username').value.trim();
    const password = document.getElementById('password').value.trim();
    if (!username || !password) {
        showToast('请输入用户名和密码', 'error');
        return;
    }
    const result = await apiRequest('/admin/login', {
        method: 'POST',
        body: JSON.stringify({ username, password })
    });
    if (result && result.success) {
        getLoginModal().hide();
        showToast('登录成功', 'success');
        switchPage('departments');
    } else {
        document.getElementById('loginError').classList.remove('d-none');
    }
});

document.getElementById('logoutBtn').addEventListener('click', async function () {
    const result = await apiRequest('/admin/logout', { method: 'POST' });
    if (result && result.success) {
        showToast('已退出登录', 'success');
        getLoginModal().show();
    }
});

async function checkLogin() {
    const result = await apiRequest('/admin/api/departments', { method: 'GET' });
    return result && result.success;
}

/* ===================================================================
   页面切换
   ===============================================================*/

const pageLoaders = {
    departments: loadDepartments,
    doctors: loadDoctors,
    schedules: loadSchedules,
    registrations: loadRegistrations,
    dashboard: loadDashboardWithChart,
    logs: loadOperationLogs,
    clients: loadClients
};

function switchPage(page) {
    // 更新菜单高亮
    document.querySelectorAll('.sidebar .nav-link').forEach(l => l.classList.remove('active'));
    document.querySelector(`.sidebar .nav-link[data-page="${page}"]`)?.classList.add('active');
    // 切换页面区块
    document.querySelectorAll('.page-section').forEach(s => s.classList.add('d-none'));
    const target = document.getElementById('page-' + page);
    if (target) {
        target.classList.remove('d-none');
        // 调用对应加载器
        if (pageLoaders[page]) pageLoaders[page]();
    }
}

document.querySelectorAll('.sidebar .nav-link').forEach(link => {
    link.addEventListener('click', function (e) {
        e.preventDefault();
        switchPage(this.dataset.page);
    });
});

/* ===================================================================
   科室管理
   ===============================================================*/

async function loadDepartments() {
    const tbody = document.getElementById('deptTableBody');
    tbody.innerHTML = '<tr><td colspan="5" class="text-center text-muted">加载中...</td></tr>';
    const result = await apiRequest('/admin/api/departments', { method: 'GET' });
    if (!result || !result.success) {
        tbody.innerHTML = '<tr><td colspan="5" class="text-center text-danger">加载失败</td></tr>';
        return;
    }
    if (result.data.length === 0) {
        tbody.innerHTML = '<tr><td colspan="5" class="text-center text-muted">暂无科室数据</td></tr>';
        return;
    }
    tbody.innerHTML = result.data.map(d => `
        <tr>
            <td>${d.id}</td>
            <td><strong>${escapeHtml(d.name)}</strong></td>
            <td>${escapeHtml(d.description || '-')}</td>
            <td><span class="${d.is_active ? 'status-active' : 'status-inactive'}">${d.is_active ? '启用' : '停用'}</span></td>
            <td>
                <button class="btn btn-sm btn-outline-primary me-1" onclick="editDept(${d.id},'${escapeHtml(d.name)}','${escapeHtml(d.description || '')}',${d.is_active})">✏️ 编辑</button>
                <button class="btn btn-sm btn-outline-danger" onclick="confirmDeleteDept(${d.id},'${escapeHtml(d.name)}')">🗑️ 删除</button>
            </td>
        </tr>
    `).join('');
}

document.getElementById('addDeptBtn').addEventListener('click', function () {
    document.getElementById('deptModalTitle').textContent = '新增科室';
    document.getElementById('deptId').value = '';
    document.getElementById('deptName').value = '';
    document.getElementById('deptDesc').value = '';
    document.getElementById('deptActive').checked = true;
});

function editDept(id, name, description, isActive) {
    document.getElementById('deptModalTitle').textContent = '编辑科室';
    document.getElementById('deptId').value = id;
    document.getElementById('deptName').value = name;
    document.getElementById('deptDesc').value = description;
    document.getElementById('deptActive').checked = isActive;
    new bootstrap.Modal(document.getElementById('deptModal')).show();
}

document.getElementById('saveDeptBtn').addEventListener('click', async function () {
    const id = document.getElementById('deptId').value;
    const name = document.getElementById('deptName').value.trim();
    const description = document.getElementById('deptDesc').value.trim();
    const isActive = document.getElementById('deptActive').checked;
    if (!name) { showToast('请输入科室名称', 'error'); return; }

    const url = '/admin/api/departments';
    const method = id ? 'PUT' : 'POST';
    const body = id
        ? JSON.stringify({ id: parseInt(id), name, description, is_active: isActive })
        : JSON.stringify({ name, description, is_active: isActive });

    const result = await apiRequest(url, { method, body });
    if (result && result.success) {
        bootstrap.Modal.getInstance(document.getElementById('deptModal')).hide();
        showToast(id ? '科室更新成功' : '科室创建成功', 'success');
        loadDepartments();
    } else {
        showToast(result?.message || '操作失败', 'error');
    }
});

function confirmDeleteDept(id, name) {
    window._deleteContext = { type: 'dept', id };
    document.getElementById('deleteModalBody').innerHTML = `确定要删除科室 <strong>${escapeHtml(name)}</strong> 吗？此操作不可恢复。`;
    new bootstrap.Modal(document.getElementById('deleteModal')).show();
}

/* ===================================================================
   医生管理
   ===============================================================*/

let departmentsCache = [];

async function loadDoctorDeptOptions(selectedId) {
    const sel = document.getElementById('doctorDept');
    if (departmentsCache.length === 0) {
        const result = await apiRequest('/admin/api/departments', { method: 'GET' });
        if (result && result.success) departmentsCache = result.data;
    }
    sel.innerHTML = '<option value="">选择科室</option>';
    departmentsCache.forEach(d => {
        const opt = document.createElement('option');
        opt.value = d.id;
        opt.textContent = d.name;
        if (selectedId && d.id === selectedId) opt.selected = true;
        sel.appendChild(opt);
    });
}

async function loadDoctors() {
    const tbody = document.getElementById('doctorTableBody');
    tbody.innerHTML = '<tr><td colspan="6" class="text-center text-muted">加载中...</td></tr>';
    const result = await apiRequest('/admin/api/doctors', { method: 'GET' });
    if (!result || !result.success) {
        tbody.innerHTML = '<tr><td colspan="6" class="text-center text-danger">加载失败</td></tr>';
        return;
    }
    if (result.data.length === 0) {
        tbody.innerHTML = '<tr><td colspan="6" class="text-center text-muted">暂无医生数据</td></tr>';
        return;
    }
    tbody.innerHTML = result.data.map(d => `
        <tr>
            <td>${d.id}</td>
            <td><strong>${escapeHtml(d.name)}</strong></td>
            <td>${escapeHtml(d.department_name || '-')}</td>
            <td>${escapeHtml(d.title || '-')}</td>
            <td>${escapeHtml(d.specialty || '-')}</td>
            <td>
                <button class="btn btn-sm btn-outline-primary me-1" onclick="editDoctor(${d.id})">✏️ 编辑</button>
                <button class="btn btn-sm btn-outline-danger" onclick="confirmDeleteDoctor(${d.id},'${escapeHtml(d.name)}')">🗑️ 删除</button>
            </td>
        </tr>
    `).join('');
}

document.getElementById('addDoctorBtn').addEventListener('click', function () {
    document.getElementById('doctorModalTitle').textContent = '新增医生';
    document.getElementById('doctorId').value = '';
    document.getElementById('doctorName').value = '';
    document.getElementById('doctorTitle').value = '';
    document.getElementById('doctorSpecialty').value = '';
    loadDoctorDeptOptions(null);
});

async function editDoctor(id) {
    document.getElementById('doctorModalTitle').textContent = '编辑医生';
    document.getElementById('doctorId').value = id;
    const result = await apiRequest('/admin/api/doctors', { method: 'GET' });
    if (!result || !result.success) return;
    const doc = result.data.find(d => d.id === id);
    if (!doc) { showToast('医生数据不存在', 'error'); return; }
    document.getElementById('doctorName').value = doc.name;
    document.getElementById('doctorTitle').value = doc.title || '';
    document.getElementById('doctorSpecialty').value = doc.specialty || '';
    await loadDoctorDeptOptions(doc.department_id);
    new bootstrap.Modal(document.getElementById('doctorModal')).show();
}

document.getElementById('saveDoctorBtn').addEventListener('click', async function () {
    const id = document.getElementById('doctorId').value;
    const name = document.getElementById('doctorName').value.trim();
    const department_id = parseInt(document.getElementById('doctorDept').value);
    const title = document.getElementById('doctorTitle').value.trim();
    const specialty = document.getElementById('doctorSpecialty').value.trim();

    if (!name) { showToast('请输入医生姓名', 'error'); return; }
    if (!department_id) { showToast('请选择所属科室', 'error'); return; }

    const url = '/admin/api/doctors';
    const method = id ? 'PUT' : 'POST';
    const body = JSON.stringify(id
        ? { id: parseInt(id), name, department_id, title, specialty }
        : { name, department_id, title, specialty }
    );

    const result = await apiRequest(url, { method, body });
    if (result && result.success) {
        bootstrap.Modal.getInstance(document.getElementById('doctorModal')).hide();
        showToast(id ? '医生信息更新成功' : '医生创建成功', 'success');
        loadDoctors();
    } else {
        showToast(result?.message || '操作失败', 'error');
    }
});

function confirmDeleteDoctor(id, name) {
    window._deleteContext = { type: 'doctor', id };
    document.getElementById('deleteModalBody').innerHTML = `确定要删除医生 <strong>${escapeHtml(name)}</strong> 吗？此操作不可恢复。`;
    new bootstrap.Modal(document.getElementById('deleteModal')).show();
}

/* ===================================================================
   号源管理
   ===============================================================*/

let scheduleEditId = null;

// ??????
async function loadSchedules() {
    const tbody = document.getElementById('scheduleTableBody');
    if (!tbody) return;
    tbody.innerHTML = '<tr><td colspan="8" class="text-center text-muted">加载中...</td></tr>';
    
    const result = await apiRequest('/admin/api/schedules', { method: 'GET' });
    if (!result || !result.success) {
        tbody.innerHTML = '<tr><td colspan="8" class="text-center text-danger">加载失败</td></tr>';
        return;
    }
    
    // 同时加载医生选项到下拉框
    await loadScheduleDoctorOptions();
    
    if (result.data.length === 0) {
        tbody.innerHTML = '<tr><td colspan="8" class="text-center text-muted">暂无号源配置</td></tr>';
        return;
    }
    
    tbody.innerHTML = result.data.map(function(s) {
        var rc = s.remaining_slots === 0 ? 'text-danger fw-bold' : 'text-success fw-bold';
        var safeDocName = escapeHtml(s.doctor_name || '未知');
        var safeDeptName = escapeHtml(s.department_name || '未知');
        return '<tr>' +
            '<td>' + s.id + '</td>' +
            '<td><strong>' + safeDocName + '</strong></td>' +
            '<td>' + safeDeptName + '</td>' +
            '<td>' + s.date + '</td>' +
            '<td>' + s.total_slots + '</td>' +
            '<td><span class="' + rc + '">' + s.remaining_slots + '</span></td>' +
            '<td>' + s.start_time + ' - ' + s.end_time + '</td>' +
            '<td>' +
                '<button class="btn btn-sm btn-outline-primary me-1" onclick="editSchedule(' + s.id + ')">编辑</button>' +
                '<button class="btn btn-sm btn-outline-danger" onclick="deleteSchedule(' + s.id + ')">删除</button>' +
            '</td>' +
        '</tr>';
    }).join('');
}

// 加载医生选项到号源模态框
async function loadScheduleDoctorOptions() {
    const sel = document.getElementById('scheduleDoctor');
    if (!sel) return;
    sel.innerHTML = '<option value="">选择医生</option>';
    const result = await apiRequest('/admin/api/doctors', { method: 'GET' });
    if (result && result.success) {
        result.data.forEach(function(d) {
            var opt = document.createElement('option');
            opt.value = d.id;
            opt.textContent = d.name + ' (' + (d.department_name || '未分配') + ')';
            sel.appendChild(opt);
        });
    }
}

// 显示新增号源表单
function showScheduleForm() {
    document.getElementById('scheduleModalTitle').textContent = '新增号源';
    document.getElementById('scheduleId').value = '';
    document.getElementById('scheduleDoctor').value = '';
    document.getElementById('scheduleDate').value = '';
    document.getElementById('scheduleTotal').value = 10;
    document.getElementById('scheduleStart').value = '08:00';
    document.getElementById('scheduleEnd').value = '17:00';
    scheduleEditId = null;
    new bootstrap.Modal(document.getElementById('scheduleModal')).show();
}

// 编辑号源
async function editSchedule(id) {
    const result = await apiRequest('/admin/api/schedules', { method: 'GET' });
    if (!result || !result.success) return;
    var found = null;
    for (var i = 0; i < result.data.length; i++) {
        if (result.data[i].id === id) { found = result.data[i]; break; }
    }
    if (!found) {
        showToast('号源数据不存在', 'error');
        return;
    }
    document.getElementById('scheduleModalTitle').textContent = '编辑号源';
    document.getElementById('scheduleId').value = found.id;
    document.getElementById('scheduleDoctor').value = found.doctor_id;
    document.getElementById('scheduleDate').value = found.date;
    document.getElementById('scheduleTotal').value = found.total_slots;
    document.getElementById('scheduleStart').value = found.start_time;
    document.getElementById('scheduleEnd').value = found.end_time;
    scheduleEditId = id;
    new bootstrap.Modal(document.getElementById('scheduleModal')).show();
}

// 保存号源
async function saveSchedule() {
    var doctorId = document.getElementById('scheduleDoctor').value;
    var dateVal = document.getElementById('scheduleDate').value;
    var totalSlots = parseInt(document.getElementById('scheduleTotal').value) || 10;
    var startTime = document.getElementById('scheduleStart').value;
    var endTime = document.getElementById('scheduleEnd').value;
    
    if (!doctorId) {
        showToast('请选择医生', 'error');
        return;
    }
    if (!dateVal) {
        showToast('请选择日期', 'error');
        return;
    }
    
    var data = {
        doctor_id: parseInt(doctorId),
        date: dateVal,
        total_slots: totalSlots,
        start_time: startTime,
        end_time: endTime
    };
    
    var url = '/admin/api/schedules';
    var method = 'POST';
    if (scheduleEditId) {
        data.id = scheduleEditId;
        method = 'PUT';
    }
    
    var result = await apiRequest(url, { method: method, body: JSON.stringify(data) });
    if (result && result.success) {
        bootstrap.Modal.getInstance(document.getElementById('scheduleModal')).hide();
        showToast(scheduleEditId ? '号源更新成功' : '号源创建成功', 'success');
        scheduleEditId = null;
        loadSchedules();
    } else {
        var msg = (result && result.message) || '操作失败';
        showToast(msg, 'error');
    }
}

// 删除号源
async function deleteSchedule(id) {
    if (!confirm('确定要删除该号源配置吗？')) return;
    var result = await apiRequest('/admin/api/schedules?id=' + id, { method: 'DELETE' });
    if (result && result.success) {
        showToast('号源已删除', 'success');
        loadSchedules();
    } else {
        var msg = (result && result.message) || '删除失败';
        showToast(msg, 'error');
    }
}

// 号源保存按钮事件绑定 - 使用延迟确保 DOM 加载完成
setTimeout(function() {
    var btn = document.getElementById('saveScheduleBtn');
    if (btn) btn.addEventListener('click', saveSchedule);
}, 100);
/* ===================================================================
   通用删除
   ===============================================================*/

document.getElementById('confirmDeleteBtn').addEventListener('click', async function () {
    const ctx = window._deleteContext;
    if (!ctx) return;
    let url = '';
    if (ctx.type === 'dept') url = '/admin/api/departments?id=' + ctx.id;
    else if (ctx.type === 'doctor') url = '/admin/api/doctors?id=' + ctx.id;
    if (!url) return;
    const result = await apiRequest(url, { method: 'DELETE' });
    if (result && result.success) {
        bootstrap.Modal.getInstance(document.getElementById('deleteModal')).hide();
        showToast('删除成功', 'success');
        window._deleteContext = null;
        if (ctx.type === 'dept') loadDepartments();
        else if (ctx.type === 'doctor') loadDoctors();
    } else {
        showToast(result?.message || '删除失败', 'error');
    }
});

/* ===================================================================
   挂号记录
   ===============================================================*/

async function loadRegistrations() {
    const tbody = document.getElementById('regTableBody');
    const deptFilter = document.getElementById('regDeptFilter').value;
    const dateFilter = document.getElementById('regDateFilter').value;

    tbody.innerHTML = '<tr><td colspan="8" class="text-center text-muted">加载中...</td></tr>';

    // 加载科室筛选下拉（首次）
    const deptSel = document.getElementById('regDeptFilter');
    if (deptSel.options.length <= 1) {
        const deptResult = await apiRequest('/admin/api/departments', { method: 'GET' });
        if (deptResult && deptResult.success) {
            deptResult.data.forEach(d => {
                const opt = document.createElement('option');
                opt.value = d.name;
                opt.textContent = d.name;
                if (deptFilter && d.name === deptFilter) opt.selected = true;
                deptSel.appendChild(opt);
            });
        }
    }

    // 构造查询参数
    const params = new URLSearchParams();
    if (deptFilter) params.append('department', deptFilter);
    if (dateFilter) params.append('date', dateFilter);
    const qs = params.toString();

    // 获取挂号记录
    const result = await apiRequest('/admin/api/registrations' + (qs ? '?' + qs : ''), { method: 'GET' });
    if (!result || !result.success) {
        tbody.innerHTML = '<tr><td colspan="8" class="text-center text-danger">加载失败</td></tr>';
        return;
    }
    if (result.data.length === 0) {
        tbody.innerHTML = '<tr><td colspan="8" class="text-center text-muted">暂无挂号记录</td></tr>';
        return;
    }

    // 渲染表格
    tbody.innerHTML = result.data.map(r => {
        const canChange = r.status === 'waiting';
        return `<tr>
            <td>${escapeHtml(r.ticket_id)}</td>
            <td>${escapeHtml(r.patient_name)}</td>
            <td>${escapeHtml(r.department)}</td>
            <td>${escapeHtml(r.doctor)}</td>
            <td>${escapeHtml(r.date)}</td>
            <td>
                <select class="form-select form-select-sm status-select" data-id="${r.id}">
                    <option value="waiting" ${r.status === 'waiting' ? 'selected' : ''}>待就诊</option>
                    <option value="completed" ${r.status === 'completed' ? 'selected' : ''}>已完成</option>
                    <option value="cancelled" ${r.status === 'cancelled' ? 'selected' : ''}>已取消</option>
                </select>
            </td>
            <td>${r.created_at ? new Date(r.created_at).toLocaleTimeString('zh-CN', { hour12: false }) : '-'}</td>
            <td>
                <button class="btn btn-sm btn-outline-primary me-1" onclick="viewRegistration('${escapeHtml(r.ticket_id)}')">📋 查看</button>
                ${canChange ? `<button class="btn btn-sm btn-outline-danger" onclick="cancelRegistration(${r.id})">✖ 取消</button>` : ''}
            </td>
        </tr>`;
    }).join('');

    // ===== 绑定状态下拉框事件 =====
    document.querySelectorAll('.status-select').forEach(sel => {
        sel.addEventListener('change', function() {
            const id = this.dataset.id;
            const newStatus = this.value;
            updateRegistrationStatus(id, newStatus);
        });
    });
}

// 独立的事件处理函数
function handleStatusChange(e) {
    const sel = e.target;
    const id = sel.dataset.id;
    if (id) {
        updateRegistrationStatus(id, sel.value);
    } else {
        console.error('状态选择器缺少 data-id');
        showToast('系统错误：缺少挂号ID', 'error');
    }
}

// ========== 挂号记录增强 ==========
// 更新挂号状态
async function updateRegistrationStatus(id, newStatus) {
    const result = await apiRequest(`/admin/api/registrations/${id}`, {
        method: 'PUT',
        body: JSON.stringify({ status: newStatus })
    });
    if (result && result.success) {
        showToast('状态更新成功', 'success');
        loadRegistrations();
    } else {
        showToast(result?.message || '更新失败', 'error');
    }
}

// 取消挂号（快速操作）
async function cancelRegistration(id) {
    if (!confirm('确定要取消该挂号吗？号源将被释放。')) return;
    await updateRegistrationStatus(id, 'cancelled');
}

// 查看挂号详情
async function viewRegistration(ticketId) {
    // 先从当前列表中找到对应记录（或重新请求）
    const result = await apiRequest('/admin/api/registrations', { method: 'GET' });
    if (!result || !result.success) {
        showToast('获取详情失败', 'error');
        return;
    }
    const reg = result.data.find(r => r.ticket_id === ticketId);
    if (!reg) {
        showToast('未找到该挂号记录', 'error');
        return;
    }

    // 填充模态框内容
    const body = document.getElementById('registrationDetailBody');
    body.innerHTML = `
        <div class="row">
            <div class="col-md-6"><strong>单号：</strong>${escapeHtml(reg.ticket_id)}</div>
            <div class="col-md-6"><strong>患者：</strong>${escapeHtml(reg.patient_name)}</div>
            <div class="col-md-6"><strong>科室：</strong>${escapeHtml(reg.department)}</div>
            <div class="col-md-6"><strong>医生：</strong>${escapeHtml(reg.doctor)}</div>
            <div class="col-md-6"><strong>日期：</strong>${escapeHtml(reg.date)}</div>
            <div class="col-md-6"><strong>状态：</strong>${escapeHtml(reg.status)}</div>
            <div class="col-12"><strong>症状：</strong>${escapeHtml(reg.symptoms)}</div>
            <div class="col-12"><strong>创建时间：</strong>${escapeHtml(reg.created_at || '')}</div>
        </div>
    `;

    // 显示模态框
    new bootstrap.Modal(document.getElementById('registrationDetailModal')).show();
}

// 筛选按钮
document.getElementById('regFilterBtn').addEventListener('click', function () {
    loadRegistrations();
});
document.getElementById('regResetBtn').addEventListener('click', function () {
    document.getElementById('regDeptFilter').value = '';
    document.getElementById('regDateFilter').value = '';
    loadRegistrations();
});

/* ===================================================================
   仪表盘
   ===============================================================*/

async function loadDashboard() {
    const result = await apiRequest('/admin/api/stats', { method: 'GET' });
    if (!result || !result.success) return;
    document.getElementById('statTodayRegs').textContent = result.today_registrations;
    document.getElementById('statTotalRegs').textContent = result.total_registrations;
    const container = document.getElementById('hotDeptList');
    if (!result.hot_departments || result.hot_departments.length === 0) {
        container.innerHTML = '<p class="text-muted text-center">暂无数据</p>';
        return;
    }
    const maxCount = Math.max(...result.hot_departments.map(d => d.count), 1);
    container.innerHTML = result.hot_departments.map((d, i) => {
        const pct = (d.count / maxCount * 100).toFixed(0);
        return `<div class="hot-dept-item">
            <div class="hot-dept-rank">${i + 1}</div>
            <div class="hot-dept-name">${escapeHtml(d.name)}</div>
            <div class="hot-dept-bar"><div class="hot-dept-bar-inner" style="width:${pct}%"></div></div>
            <div class="hot-dept-count">${d.count} 次</div>
        </div>`;
    }).join('');
}


/* ===================================================================
   页面初始化
   ===============================================================*/

/* ===================================================================
   CSV 导出
   =================================================================== */
async function exportRegistrationsCSV() {
    const deptFilter = document.getElementById('regDeptFilter').value;
    const dateFilter = document.getElementById('regDateFilter').value;
    const params = new URLSearchParams();
    if (deptFilter) params.append('department', deptFilter);
    if (dateFilter) params.append('date', dateFilter);
    const qs = params.toString();
    const result = await apiRequest('/admin/api/registrations' + (qs ? '?' + qs : ''), { method: 'GET' });
    if (!result || !result.success || !result.data || result.data.length === 0) {
        showToast('没有可导出的数据', 'error');
        return;
    }
    const statusMap = { waiting: '待就诊', completed: '已完成', cancelled: '已取消' };
    let csvContent = '\uFEFF';
    csvContent += '单号,患者,科室,医生,日期,症状,状态,创建时间\n';
    result.data.forEach(function(r) {
        var row = [
            r.ticket_id,
            r.patient_name,
            r.department,
            r.doctor,
            r.date,
            (r.symptoms || '').replace(/,/g, '|').replace(/\n/g, ' '),
            statusMap[r.status] || r.status,
            r.created_at ? new Date(r.created_at).toLocaleTimeString('zh-CN', { hour12: false }) : ''
        ];
        csvContent += row.join(',') + '\n';
    });
    var blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
    var url = URL.createObjectURL(blob);
    var a = document.createElement('a');
    a.href = url;
    a.download = '挂号记录_' + new Date().toISOString().slice(0, 10) + '.csv';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
    showToast('导出成功: ' + result.data.length + ' 条记录', 'success');
}

/* ===================================================================
   仪表盘 - 近7天趋势图
   =================================================================== */
let trendChartInstance = null;

async function loadTrendChart() {
    const canvas = document.getElementById('trendChart');
    if (!canvas) return;
    const result = await apiRequest('/admin/api/trend', { method: 'GET' });
    if (!result || !result.success || !result.data) return;
    const labels = result.data.map(function(d) { return d.day; });
    const counts = result.data.map(function(d) { return d.count; });
    if (typeof Chart === 'undefined') {
        // Chart.js 未加载，显示提示
        canvas.parentElement.innerHTML = '<p class="text-muted text-center">Chart.js 未加载，请检查网络</p>';
        return;
    }
    if (trendChartInstance) trendChartInstance.destroy();
    var ctx = canvas.getContext('2d');
    trendChartInstance = new Chart(ctx, {
        type: 'line',
        data: {
            labels: labels,
            datasets: [{
                label: '挂号数量',
                data: counts,
                borderColor: '#1a73e8',
                backgroundColor: 'rgba(26, 115, 232, 0.1)',
                fill: true,
                tension: 0.3,
                pointBackgroundColor: '#1a73e8',
                pointRadius: 4,
                pointHoverRadius: 6
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: { legend: { display: false } },
            scales: {
                y: { beginAtZero: true, ticks: { stepSize: 1, precision: 0 } }
            }
        }
    });
}

// 重写 loadDashboard 以包含趋势图
function loadDashboardWithChart() {
    loadDashboard();
    setTimeout(loadTrendChart, 300);
}

/* ===================================================================
   操作日志
   =================================================================== */
async function loadOperationLogs() {
    const tbody = document.getElementById('logTableBody'); 
    if (!tbody) return;
    tbody.innerHTML = '<tr><td colspan="4" class="text-center text-muted">加载中...</td></tr>';
    const result = await apiRequest('/admin/api/logs', { method: 'GET' });
    if (!result || !result.success) {
        tbody.innerHTML = '<tr><td colspan="4" class="text-center text-danger">加载失败</td></tr>';
        return;
    }
    if (result.data.length === 0) {
        tbody.innerHTML = '<tr><td colspan="4" class="text-center text-muted">暂无操作日志</td></tr>';
        return;
    }
    var actionMap = {
        'login': '登录', 'logout': '退出',
        'add_dept': '新增科室', 'edit_dept': '编辑科室', 'del_dept': '删除科室',
        'add_doctor': '新增医生', 'edit_doctor': '编辑医生', 'del_doctor': '删除医生',
        'add_schedule': '新增号源', 'edit_schedule': '编辑号源', 'del_schedule': '删除号源'
    };
    var classMap = {
        'login': 'log-action-info', 'logout': 'log-action-info',
        'add_dept': 'log-action-success', 'add_doctor': 'log-action-success', 'add_schedule': 'log-action-success',
        'edit_dept': 'log-action-warning', 'edit_doctor': 'log-action-warning', 'edit_schedule': 'log-action-warning',
        'del_dept': 'log-action-danger', 'del_doctor': 'log-action-danger', 'del_schedule': 'log-action-danger'
    };
    tbody.innerHTML = result.data.map(function(log) {
        var safeTarget = escapeHtml(log.target || '-');
        var safeDetails = escapeHtml(log.details || '-');
        var actionLabel = actionMap[log.action] || log.action;
        var cls = classMap[log.action] || 'log-action-info';
        return '<tr>' +
            '<td class="text-muted small">' + escapeHtml(log.timestamp) + '</td>' +
            '<td><span class="log-action-badge ' + cls + '">' + actionLabel + '</span></td>' +
            '<td>' + safeTarget + '</td>' +
            '<td><small>' + safeDetails + '</small></td>' +
        '</tr>';
    }).join('');
    var countEl = document.getElementById('logCount');
    if (countEl) countEl.textContent = '共 ' + result.total + ' 条记录（仅显示最近200条）';
}

/* ===================================================================
   终端管理
   =================================================================== */
async function loadClients() {
    const tbody = document.getElementById('clientTableBody');
    if (!tbody) return;
    tbody.innerHTML = '<tr><td colspan="5" class="text-center text-muted">加载中...</td></tr>';
    const result = await apiRequest('/admin/api/clients', { method: 'GET' });
    if (!result || !result.success) {
        tbody.innerHTML = '<tr><td colspan="5" class="text-center text-danger">加载失败</td></tr>';
        return;
    }
    if (result.data.length === 0) {
        tbody.innerHTML = '<tr><td colspan="5" class="text-center text-muted">暂无终端</td></tr>';
        return;
    }
    tbody.innerHTML = result.data.map(c => `
        <tr>
            <td>${escapeHtml(c.id)}</td>
            <td>${escapeHtml(c.name)}</td>
            <td><code>${escapeHtml(c.api_key)}</code> <button class="btn btn-sm btn-outline-secondary" onclick="copyKey('${escapeHtml(c.api_key)}')">📋 复制</button></td>
            <td><span class="${c.is_active ? 'status-active' : 'status-inactive'}">${c.is_active ? '启用' : '禁用'}</span></td>
            <td>
                <button class="btn btn-sm btn-outline-primary me-1" onclick="toggleClient(${c.id}, ${!c.is_active})">${c.is_active ? '禁用' : '启用'}</button>
                <button class="btn btn-sm btn-outline-danger" onclick="deleteClient(${c.id})">删除</button>
            </td>
        </tr>
    `).join('');
}

// 复制 Key
function copyKey(key) {
    navigator.clipboard.writeText(key).then(() => {
        showToast('已复制 API Key', 'success');
    });
}

// 切换终端状态
async function toggleClient(id, newStatus) {
    const result = await apiRequest('/admin/api/clients', {
        method: 'PUT',
        body: JSON.stringify({ id, is_active: newStatus })
    });
    if (result && result.success) {
        showToast('状态更新成功', 'success');
        loadClients();
    } else {
        showToast(result?.message || '操作失败', 'error');
    }
}

// 删除终端
async function deleteClient(id) {
    if (!confirm('确定要删除该终端吗？')) return;
    const result = await apiRequest(`/admin/api/clients?id=${id}`, { method: 'DELETE' });
    if (result && result.success) {
        showToast('删除成功', 'success');
        loadClients();
    } else {
        showToast(result?.message || '删除失败', 'error');
    }
}

// 初始化
(async function init() {
    const loggedIn = await checkLogin();
    if (loggedIn) {
        switchPage('departments');
    } else {
        getLoginModal().show();
    }
})();
