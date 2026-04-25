// 【关键】全局变量，用于存储数据，解决勾选不生效和备注覆盖问题
let currentListData = []; 

// 页面加载完成后获取数据
document.addEventListener('DOMContentLoaded', fetchData);
document.addEventListener('DOMContentLoaded', () => {
    const addBtn = document.getElementById('addBtn');
    if (addBtn) addBtn.onclick = openModal; // 绑定打开函数
    fetchData(); 
});

// 从后端获取数据
async function fetchData() {
    try {
        // 加入时间戳解决“刷新多次才生效”的缓存问题
        const response = await fetch(`/api/getData?t=${Date.now()}`,{
            method: 'GET',
            headers: {
                'Auth-Token': await getToken()
            }            
        });
        const data = await response.json();
        
        // 【核心修复】将数据存入全局变量，供 updateTaskState 使用
        currentListData = data.list || []; 
        
        renderList(currentListData);
    } catch (err) {
        console.error("加载失败:", err);
    }
}

// 渲染列表函数
function renderList(list) {
    const container = document.getElementById('scheduleList');
    if (!container) return;
    container.innerHTML = '';

    const now = new Date();
    const nowTs = Math.floor(now.getTime() / 1000); 
    
    // 获取时间阈值
    const todayStart = new Date(now.getFullYear(), now.getMonth(), now.getDate()).getTime() / 1000;
    const tomorrowStart = todayStart + 86400; 
    const fifteenDaysLater = todayStart + (15 * 86400); 

    // 1. 按开始时间排序 (创建副本避免影响全局变量顺序)
    const sortedList = [...list].sort((a, b) => a.beginTime - b.beginTime);

    sortedList.forEach(item => {
        const card = document.createElement('div');
        const isDone = item.state === 1;
        const taskTime = item.beginTime;
        
        // --- 核心颜色判定逻辑 ---
        let statusClass = '';
        if (isDone) {
            // 已完成逻辑
            if (taskTime < todayStart) {
                statusClass = 'status-past-done';   // 昨日之前已完成：翡翠绿
            } else {
                statusClass = 'status-today-done';  // 今日已完成：深绿色
            }
        } else {
            // 未完成逻辑
            if (nowTs > taskTime) {
                statusClass = 'status-late';        // 过期：红色
            } else if (taskTime >= todayStart && taskTime < tomorrowStart) {
                statusClass = 'status-today-todo';  // 今日未过期未完成：黄色
            } else if (taskTime >= tomorrowStart && taskTime < fifteenDaysLater) {
                statusClass = 'status-near-future'; // 近期 (15天内)：蓝色
            } else {
                statusClass = 'status-far-future';  // 远期 (15天以上)：紫色
            }
        }

        const beginDateObj = new Date(item.beginTime * 1000);
        card.className = `card ${isDone ? 'completed' : ''} ${statusClass}`;
        
        card.innerHTML = `
            <input type="checkbox" class="card-checkbox" 
                ${isDone ? 'checked' : ''} 
                onchange="updateTaskState(${item.sID}, this.checked)">
            
            <div class="card-info">
                <div class="card-header-row">
                    <h3>${item.ocName}</h3>
                    <div class="countdown-tag" id="timer-${item.sID}">-</div>
                </div>
                <div class="card-details">
                    <span>📅 ${beginDateObj.toLocaleString()}</span>
                    <span>⏳ ${item.stayTime} min</span>
                </div>
                <div class="remark">${item.remark || ''}</div>
            </div>

            <div class="glow-container">
                <div class="glow-spot"></div>
            </div>
        `;
        
        container.appendChild(card);
        startTimer(item.sID, item.beginTime);
    });
}

// function renderList(list) {
//     const container = document.getElementById('scheduleList');
//     if (!container) return;
//     container.innerHTML = '';

//     list.forEach(item => {
//         const card = document.createElement('div');
//         // 初始化一次基础结构
//         card.innerHTML = `
//             <input type="checkbox" class="card-checkbox">
//             <div class="card-info">
//                 <div class="card-header-row">
//                     <h3>${item.ocName}</h3>
//                     <div class="countdown-tag">加载中...</div>
//                 </div>
//                 <div class="card-details">
//                     <span>📅 ${new Date(item.beginTime * 1000).toLocaleString()}</span>
//                     <span>⏳ ${item.stayTime} min</span>
//                 </div>
//             </div>
//             <div class="glow-container"><div class="glow-spot"></div></div>
//         `;

//         const checkbox = card.querySelector('.card-checkbox');
//         const timerTag = card.querySelector('.countdown-tag');
//         checkbox.checked = item.state === 1;
//         checkbox.onchange = (e) => updateTaskState(item.sID, e.target.checked);

//         // --- 【核心：卡片的自我检查器】 ---
//         const updateSelf = () => {
//             const nowTs = Math.floor(Date.now() / 1000);
//             const isDone = item.state === 1;
//             const left = item.beginTime - nowTs;

//             // 1. 更新颜色分类 (statusClass)
//             // 这里直接操作 className，不需要重新销毁 DOM
//             let statusClass = getStatusClass(item, nowTs); 
//             card.className = `card ${isDone ? 'completed' : ''} ${statusClass}`;

//             // 2. 更新倒计时文字
//             if (isDone) {
//                 timerTag.innerText = "已完成";
//                 return true; // 已完成的任务可以停止心跳了
//             } else if (left <= 0) {
//                 timerTag.innerText = "已过期";
//             } else {
//                 const h = Math.floor(left / 3600);
//                 const m = Math.floor((left % 3600) / 60);
//                 const s = left % 60;
//                 timerTag.innerText = `倒计时: ${h}h ${m}m ${s}s`;
//             }
//             return false;
//         };

//         // 立即执行一次
//         updateSelf();

//         // 开启这个卡片的专属心跳
//         const selfInterval = setInterval(() => {
//             // 如果卡片在 DOM 里消失了（被删了），或者 updateSelf 返回 true，就停止心跳
//             if (!document.contains(card) || updateSelf()) {
//                 clearInterval(selfInterval);
//             }
//         }, 1000);

//         container.appendChild(card);
//     });
// }

// 辅助函数：把颜色判定逻辑抽出来，让代码更好看
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
    // 使用 == 兼容可能存在的字符串或数字类型差异
    const task = currentListData.find(item => item.sID == sID);
    
    if (!task) {
        console.error("未找到任务数据，无法执行勾选");
        return;
    }

    const newState = isDone ? 1 : 0;

    const payload = {
        sID: sID,
        state: newState,
        status: newState, 
        ocName: task.ocName,     
        remark: task.remark,     // 关键：保留原备注，不使用 "Updated by web"
        beginTime: task.beginTime,
        stayTime: task.stayTime
    };

    try {
        // 乐观 UI 更新：先在界面上变色，提升响应感
        task.state = newState;
        task.status = newState;
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
            // 后台静默刷新以确保与服务器完全同步
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
    const now = new Date();
    document.getElementById('beginDate').value = now.toISOString().split('T')[0];
    document.getElementById('hr').value = String(now.getHours()).padStart(2, '0');
    document.getElementById('min').value = String(now.getMinutes()).padStart(2, '0');
    document.getElementById('sec').value = String(now.getSeconds()).padStart(2, '0');
    document.getElementById('modal').style.display = 'block';
}

function closeModal() {
    document.getElementById('modal').style.display = 'none';
}

// 提交新任务
async function submitData() {
    const ocName = document.getElementById('ocName').value;
    const datePart = document.getElementById('beginDate').value; 
    const hr = document.getElementById('hr').value || "00";
    const min = document.getElementById('min').value || "00";
    const sec = document.getElementById('sec').value || "00";
    const stayTime = parseInt(document.getElementById('stayTime').value) || 0;
    const remark = document.getElementById('remark').value;

    const timeFullStr = `${datePart}T${hr.padStart(2,'0')}:${min.padStart(2,'0')}:${sec.padStart(2,'0')}`;
    const timestamp = Math.floor(new Date(timeFullStr).getTime() / 1000);

    if (!ocName || isNaN(timestamp)) {
        alert("请填写任务名称和日期");
        return;
    }

    const payload = {
        ocName,
        beginTime: timestamp,
        stayTime,
        status: 0,
        state: 0,
        remark
    };

    const res = await fetch('/api/addData', {
        method: 'POST',
        headers: { 
            'Content-Type': 'application/json',
            'Auth-Token': await getToken()
         },
        body: JSON.stringify(payload)
    });

    if (res.ok) {
        closeModal();
        fetchData(); 
    }
}