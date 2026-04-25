// 【关键】全局变量，用于存储数据，解决勾选不生效和备注覆盖问题
let currentListData = []; 
const successAudio = new Audio('./music.mp3');
let currentSwipedCard = null; // 全局记录当前滑开的卡片
let editingSID = null;      // 全局记录正在编辑的任务ID
// 页面加载完成后获取数据
// 禁用全网页右键菜单
document.addEventListener('contextmenu', event => event.preventDefault());
document.addEventListener('DOMContentLoaded', fetchData);
document.addEventListener('DOMContentLoaded', () => {
    const addBtn = document.getElementById('addBtn');
    if (addBtn) addBtn.onclick = openModal; // 绑定打开函数
    fetchData(); 
});

// 从后端获取数据
async function fetchData() {
    try {
        const response = await fetch(`/api/getData?t=${Date.now()}`, {
            method: 'GET',
            headers: { 'Auth-Token': await getToken() }            
        });
        const data = await response.json();
        currentListData = data.list || []; 
        renderList(currentListData);
    } catch (err) {
        console.error("加载失败:", err);
    }
}

// 渲染列表函数
// function renderList(list) {
//     const container = document.getElementById('scheduleList');
//     if (!container) return;
//     container.innerHTML = '';

//     // 按开始时间排序
//     const sortedList = [...list].sort((a, b) => a.beginTime - b.beginTime);

//     sortedList.forEach(item => {
//         const card = document.createElement('div');
        
//         // 1. 初始化静态结构
//         const beginDateObj = new Date(item.beginTime * 1000);
//         card.innerHTML = `
//             <input type="checkbox" class="card-checkbox" 
//                 ${item.state === 1 ? 'checked' : ''}>
            
//             <div class="card-info">
//                 <div class="card-header-row">
//                     <h3>${item.ocName}</h3>
//                     <div class="countdown-tag" id="timer-${item.sID}">-</div>
//                 </div>
//                 <div class="card-details">
//                     <span>📅 ${beginDateObj.toLocaleString()}</span>
//                     <span>⏳ ${item.stayTime} min</span>
//                 </div>
//                 <div class="remark">${item.remark || ''}</div>
//             </div>

//             <div class="glow-container">
//                 <div class="glow-spot"></div>
//             </div>
//         `;

//         // 绑定复选框事件
//         const checkbox = card.querySelector('.card-checkbox');
//         checkbox.onchange = (e) => updateTaskState(item.sID, e.target.checked);

//         // 2. 【每个卡片的自我管理函数】
//         const timerTag = card.querySelector('.countdown-tag');
        
//         const updateSelfStatus = () => {
//             const nowTs = Math.floor(Date.now() / 1000);
//             const isDone = item.state === 1;
//             const beginTs = item.beginTime;
//             const endTs = beginTs + (item.stayTime * 60); // 计算结束时间戳
//             const todayStart = new Date().setHours(0,0,0,0) / 1000;

//             let statusClass = '';
//             let timerText = '';

//             if (isDone) {
//                 statusClass = (beginTs < todayStart) ? 'status-past-done' : 'status-today-done';
//                 timerText = "已完成";
//             } else {
//                 // 未完成状态下的精细判定
//                 if (nowTs < beginTs) {
//                     // 1. 尚未开始 (倒计时)
//                     if (beginTs < todayStart + 86400) statusClass = 'status-today-todo';
//                     else if (beginTs < todayStart + 15 * 86400) statusClass = 'status-near-future';
//                     else statusClass = 'status-far-future';

//                     const left = beginTs - nowTs;
//                     const h = Math.floor(left / 3600);
//                     const m = Math.floor((left % 3600) / 60);
//                     const s = left % 60;
//                     timerText = `倒计时: ${h}h ${m}m ${s}s`;
//                 } 
//                 else if (nowTs >= beginTs && nowTs <= endTs) {
//                     // 2. 正在进行中 (粉色)
//                     statusClass = 'status-running';
//                     timerText = "进行中";
//                 } 
//                 else {
//                     // 3. 已过期 (红色)
//                     statusClass = 'status-late';
//                     timerText = "已过期";
//                 }
//             }

//             // 更新 DOM
//             card.className = `card ${isDone ? 'completed' : ''} ${statusClass}`;
//             timerTag.innerText = timerText;

//             return isDone; // 如果已完成，告知外部可以考虑停止心跳（或保持检查）
//         };

//         // 3. 启动“心跳”
//         updateSelfStatus(); // 立即执行一次
//         const interval = setInterval(() => {
//             // 如果卡片已经从页面移除，清除定时器防止内存泄漏
//             if (!document.contains(card)) {
//                 clearInterval(interval);
//                 return;
//             }
//             // 执行状态检查
//             updateSelfStatus();
//         }, 1000);

//         container.appendChild(card);
//     });
// }
function renderList(list) {
    const container = document.getElementById('scheduleList');
    if (!container) return;
    container.innerHTML = '';

    const sortedList = [...list].sort((a, b) => a.beginTime - b.beginTime);

    sortedList.forEach(item => {
        const card = document.createElement('div');
        card.className = `card`; // 动态类名由 updateSelfStatus 管理

        
        // 注入新结构：操作层 + 内容层
        card.innerHTML = `
            <div class="card-actions-layer">
                <button class="action-btn btn-edit"></button>
                <button class="action-btn btn-delete"></button>
            </div>
            <div class="card-content-wrapper">
                <input type="checkbox" class="card-checkbox" ${item.state === 1 ? 'checked' : ''}>
                <div class="card-info">
                    <div class="card-header-row">
                        <h3>${item.ocName}</h3>
                        <div class="countdown-tag">-</div>
                    </div>
                    <div class="card-details">
                        <span>📅 ${new Date(item.beginTime * 1000).toLocaleString()}</span>
                        <span>⏳ ${item.stayTime} min</span>
                    </div>
                    <div class="remark">${item.remark || ''}</div>
                </div>
                <div class="more-trigger">
                    <span></span><span></span><span></span>
                </div>
                
                <div class="glow-container">
                    <div class="glow-spot"></div>
                </div>
            </div>
        `;

        // --- 功能绑定 ---

        const wrapper = card.querySelector('.card-content-wrapper');
        const trigger = card.querySelector('.more-trigger');
        const btnEdit = card.querySelector('.btn-edit');
        const btnDelete = card.querySelector('.btn-delete');
        const checkbox = card.querySelector('.card-checkbox');
        const timerTag = card.querySelector('.countdown-tag');

        // 1. 三点点击：实现滑动开关
        trigger.onclick = (e) => {
            e.stopPropagation();
            // 排他性逻辑：如果有其他卡片滑开了，先关掉它
            if (currentSwipedCard && currentSwipedCard !== card) {
                currentSwipedCard.classList.remove('swiped');
            }
            
            const isSwiping = card.classList.toggle('swiped');
            currentSwipedCard = isSwiping ? card : null;
        };

        // 2. 点击卡片主体：如果已滑开则关闭
        card.onclick = () => {
            if (card.classList.contains('swiped')) {
                card.classList.remove('swiped');
                currentSwipedCard = null;
            }
        };

        // 3. 修改按钮
        btnEdit.onclick = (e) => {
            e.stopPropagation();
            openEditModal(item); // 传入整个对象填充
            card.classList.remove('swiped');
            currentSwipedCard = null;
        };

        // 4. 删除按钮
        btnDelete.onclick = async (e) => {
            e.stopPropagation();
            if (confirm(`确定要删除任务“${item.ocName}”吗？`)) {
                await deleteTask(item.sID);
            }
            card.classList.remove('swiped');
            currentSwipedCard = null;
        };

        // 5. 复选框 (保留提示音逻辑)
        checkbox.onclick = (e) => e.stopPropagation(); // 防止触发卡片点击收起
        checkbox.onchange = (e) => updateTaskState(item.sID, e.target.checked);

        // 6. 心跳状态更新 (保留所有颜色逻辑)
        const updateSelfStatus = () => {
            const nowTs = Math.floor(Date.now() / 1000);
            const isDone = item.state === 1; // 0:未完成, 1:已完成
            const beginTs = item.beginTime;
            const endTs = beginTs + (item.stayTime * 60);
            const todayStart = new Date().setHours(0, 0, 0, 0) / 1000;

            let statusClass = '';
            let timerText = '';

            // 1. 核心状态计算逻辑 (保留原有逻辑)
            if (isDone) {
                // 已完成状态：区分是以前完成的还是今天完成的
                statusClass = (beginTs < todayStart) ? 'status-past-done' : 'status-today-done';
                timerText = "已完成";
            } else {
                if (nowTs < beginTs) {
                    // 未开始：根据距离今天的远近显示不同颜色
                    if (beginTs < todayStart + 86400) {
                        statusClass = 'status-today-todo';
                    } else if (beginTs < todayStart + 15 * 86400) {
                        statusClass = 'status-near-future';
                    } else {
                        statusClass = 'status-far-future';
                    }
                    
                    // 倒计时计算
                    const left = beginTs - nowTs;
                    const h = Math.floor(left / 3600);
                    const m = Math.floor((left % 3600) / 60);
                    const s = left % 60;
                    timerText = `倒计时: ${h}h ${m}m ${s}s`;
                } 
                else if (nowTs >= beginTs && nowTs <= endTs) {
                    // 正在进行中：触发粉色心跳和光晕
                    statusClass = 'status-running';
                    timerText = "进行中";
                } 
                else {
                    // 已过期：显示红色
                    statusClass = 'status-late';
                    timerText = "已过期";
                }
            }

            // 2. 类名同步分发 (修复圆角、光晕与滑动的冲突)
            
            // 检查当前卡片是否处于滑动打开状态，避免被 className 覆盖重置
            const swipedClass = card.classList.contains('swiped') ? 'swiped' : '';
            
            // 外层容器：仅负责滑动状态
            card.className = `card ${swipedClass}`;
            
            // 内层包装器：承载原有的所有视觉样式 (背景色、圆角、阴影、勾选滤镜、颜色状态)
            // 这样光晕 (glow-container) 作为 wrapper 的子元素，就能正确继承 statusClass 里的颜色变量
            wrapper.className = `card-content-wrapper ${isDone ? 'completed' : ''} ${statusClass}`;
            
            // 更新时间文字
            timerTag.innerText = timerText;
        };

        const interval = setInterval(() => {
            if (!document.contains(card)) { clearInterval(interval); return; }
            updateSelfStatus();
        }, 1000);
        updateSelfStatus();

        container.appendChild(card);
    });
}

// --- 新增：删除功能 ---
// --- 修改后的删除功能：改用 POST ---
async function deleteTask(sID) {
    try {
        const res = await fetch(`/api/delData`, {
            method: 'POST',
            headers: { 
                'Content-Type': 'application/json',
                'Auth-Token': await getToken() 
            },
            body: JSON.stringify({ sID: sID }) // 将 ID 放在 body 里
        });
        
        if (res.ok) {
            fetchData(); // 成功后刷新列表
        } else {
            console.error("删除失败，状态码:", res.status);
        }
    } catch (err) {
        console.error("请求出错:", err);
    }
}
// --- 新增：打开编辑模式 ---
// --- 修改后的 openEditModal ---
function openEditModal(item) {
    editingSID = item.sID; 
    
    document.getElementById('ocName').value = item.ocName;
    document.getElementById('stayTime').value = item.stayTime;
    document.getElementById('remark').value = item.remark || '';
    
    const d = new Date(item.beginTime * 1000);

    // 【核心修复】：不再使用 toISOString()，而是手动拼接本地日期
    const year = d.getFullYear();
    const month = String(d.getMonth() + 1).padStart(2, '0');
    const day = String(d.getDate()).padStart(2, '0');
    document.getElementById('beginDate').value = `${year}-${month}-${day}`;

    // 时间部分保持不变，因为 getHours() 等方法本来就是获取本地时间
    document.getElementById('hr').value = String(d.getHours()).padStart(2, '0');
    document.getElementById('min').value = String(d.getMinutes()).padStart(2, '0');
    document.getElementById('sec').value = String(d.getSeconds()).padStart(2, '0');

    document.querySelector('.modal-content h2').innerText = "修改任务";
    document.getElementById('modal').style.display = 'block';
}
function getStatusClass(item, nowTs) {
    const todayStart = new Date().setHours(0,0,0,0) / 1000;
    if (item.state === 1) {
        return (item.beginTime < todayStart) ? 'status-past-done' : 'status-today-done';
    }
    if (nowTs > item.beginTime) return 'status-late';
    if (item.beginTime < todayStart + 86400) return 'status-today-todo';
    if (item.beginTime < todayStart + 15 * 86400) return 'status-near-future';
    return 'status-far-future';
}
// 更新状态：核心修复，通过全局变量寻找原始数据，保护备注不被覆盖
async function updateTaskState(sID, isDone) {
    const task = currentListData.find(item => item.sID == sID);
    if (!task) return;

    const newState = isDone ? 1 : 0;
    
    // --- 【新逻辑】触发提示音 ---
    // 只有当用户从“未完成”切换到“已完成”时播放
    if (isDone && task.state === 0) {
        successAudio.currentTime = 0; // 重置进度，防止连点没声音
        successAudio.play().catch(err => {
            // 浏览器策略：如果用户还没在页面上点击过任何地方，可能静音
            console.warn("音频播放受阻，请确保页面有点击交互", err);
        });
    }

    // 内存数据更新
    task.state = newState;
    task.status = newState;

    const payload = {
        sID: sID,
        state: newState,
        status: newState, 
        ocName: task.ocName,     
        remark: task.remark,     
        beginTime: task.beginTime,
        stayTime: task.stayTime
    };

    try {
        // 乐观 UI 更新
        renderList(currentListData); 

        const response = await fetch('/api/upData', {
            method: 'POST',
            headers: { 
                'Content-Type': 'application/json',
                'Auth-Token': await getToken()
            },
            body: JSON.stringify(payload)
        });

        if (response.ok) {
            // 静默刷新同步
            fetchData(); 
        }
    } catch (error) {
        console.error("更新失败:", error);
    }
}

// 倒计时逻辑
function startTimer(sID, targetTime) {
    // 简单的单次定时器触发
    const interval = setInterval(() => {
        const now = Math.floor(Date.now() / 1000);
        const left = targetTime - now;
        const el = document.getElementById(`timer-${sID}`);
        
        if (!el) {
            clearInterval(interval);
            return;
        }

        if (left <= 0) {
            el.innerText = "已开始";
            clearInterval(interval);
            return;
        }
        
        const h = Math.floor(left / 3600);
        const m = Math.floor((left % 3600) / 60);
        const s = left % 60;
        el.innerText = `倒计时: ${h}h ${m}m ${s}s`;
    }, 1000);
}

// 模态框逻辑
function openModal() {
    editingSID = null; // 新增模式
    const now = new Date();
    
    document.getElementById('ocName').value = '';
    document.getElementById('remark').value = '';
    document.getElementById('stayTime').value = '';

    // --- 核心修复：手动拼接本地日期，避免时区偏移 ---
    const year = now.getFullYear();
    const month = String(now.getMonth() + 1).padStart(2, '0');
    const day = String(now.getDate()).padStart(2, '0');
    document.getElementById('beginDate').value = `${year}-${month}-${day}`;

    // 时间部分 getHours() 等方法获取的就是本地时间，保持不变
    document.getElementById('hr').value = String(now.getHours()).padStart(2, '0');
    document.getElementById('min').value = String(now.getMinutes()).padStart(2, '0');
    document.getElementById('sec').value = String(now.getSeconds()).padStart(2, '0');
    
    document.querySelector('.modal-content h2').innerText = "✨ 新增任务";
    document.getElementById('modal').style.display = 'block';
}

function closeModal() {
    document.getElementById('modal').style.display = 'none';
}

// --- 修改：提交函数适配新增/修改 ---
// --- 修改后的提交函数：适配新增/修改 ---
async function submitData() {
    // 1. 获取表单数据
    const ocName = document.getElementById('ocName').value.trim();
    const datePart = document.getElementById('beginDate').value; 
    const hr = document.getElementById('hr').value || "00";
    const min = document.getElementById('min').value || "00";
    const sec = document.getElementById('sec').value || "00";
    const stayTime = parseInt(document.getElementById('stayTime').value) || 0;
    const remark = document.getElementById('remark').value;

    // 2. 时间戳转换
    const timeFullStr = `${datePart}T${hr.padStart(2,'0')}:${min.padStart(2,'0')}:${sec.padStart(2,'0')}`;
    const timestamp = Math.floor(new Date(timeFullStr).getTime() / 1000);

    // 3. 基础校验
    if (!ocName || isNaN(timestamp)) {
        alert("请填写任务标题和完整时间");
        return;
    }

    // 4. 构建 Payload
    const payload = {
        ocName,
        beginTime: timestamp,
        stayTime,
        remark,
        status: 0, // 默认初始状态
        state: 0
    };
    
    // 【核心改动】如果是编辑模式：
    // 1. 填入 sID 告知后端更新哪一条数据
    // 2. 继承原有的完成状态（防止修改内容时把“已完成”重置为“未完成”）
    if (editingSID!=null) {
        payload.sID = editingSID;
        const original = currentListData.find(t => t.sID == editingSID);
        if (original) {
            payload.state = original.state;
            payload.status = original.status;
        }
    }

    // 5. 根据是否有 ID 选择不同的接口路径
    const url = (editingSID!=null) ? '/api/upData' : '/api/addData';
    
    try {
        const res = await fetch(url, {
            method: 'POST',
            headers: { 
                'Content-Type': 'application/json',
                'Auth-Token': await getToken()
            },
            body: JSON.stringify(payload)
        });

        if (res.ok) {
            closeModal();
            editingSID = null; // 成功后重置编辑 ID
            fetchData();       // 刷新列表
        } else {
            console.error("提交失败，状态码:", res.status);
        }
    } catch (err) {
        console.error("请求出错:", err);
    }
}