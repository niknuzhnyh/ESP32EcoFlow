import gzip

html = """<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Моніторинг Живлення</title>
<style>
:root {
  --bg-base: #0b0f19;
  --card-bg: rgba(22, 30, 49, 0.9);
  --card-border: rgba(255, 255, 255, 0.08);
  --primary: #3b82f6;
  --primary-hover: #2563eb;
  --accent-green: #10b981;
  --accent-red: #ef4444;
  --accent-yellow: #f59e0b;
  --text-main: #f3f4f6;
  --text-muted: #9ca3af;
  --input-bg: #111827;
  --input-border: #374151;
  --radius: 14px;
}
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
body { background: var(--bg-base); color: var(--text-main); min-height: 100vh; padding: 20px 14px; display: flex; justify-content: center; }
.container { width: 100%; max-width: 860px; display: flex; flex-direction: column; gap: 20px; }

header { display: flex; justify-content: space-between; align-items: center; padding: 12px 18px; background: var(--card-bg); border: 1px solid var(--card-border); border-radius: var(--radius); }
.brand { display: flex; align-items: center; gap: 12px; }
.brand-icon { width: 38px; height: 38px; border-radius: 10px; background: linear-gradient(135deg, #3b82f6, #1d4ed8); display: flex; align-items: center; justify-content: center; font-size: 20px; box-shadow: 0 4px 12px rgba(59, 130, 246, 0.35); }
.brand h1 { font-size: 1.15rem; font-weight: 700; }
.brand span { font-size: 0.75rem; color: var(--text-muted); display: block; }
.nav-tabs { display: flex; gap: 8px; }
.tab-btn { background: transparent; border: 1px solid var(--card-border); color: var(--text-muted); padding: 8px 14px; border-radius: 8px; cursor: pointer; font-size: 0.85rem; font-weight: 600; transition: 0.2s; }
.tab-btn.active, .tab-btn:hover { background: var(--primary); color: white; border-color: var(--primary); }

.card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: var(--radius); padding: 20px; }
.status-hero { text-align: center; padding: 30px 20px; display: flex; flex-direction: column; align-items: center; gap: 12px; position: relative; overflow: hidden; }
.status-pill { font-size: 0.85rem; font-weight: 700; text-transform: uppercase; letter-spacing: 1px; padding: 6px 14px; border-radius: 9999px; }
.status-hero.ok { border-color: rgba(16, 185, 129, 0.4); background: radial-gradient(circle at 50% 20%, rgba(16, 185, 129, 0.15), transparent 70%), var(--card-bg); }
.status-hero.ok .status-pill { background: rgba(16, 185, 129, 0.2); color: var(--accent-green); border: 1px solid var(--accent-green); }
.status-hero.lost { border-color: rgba(239, 68, 68, 0.5); background: radial-gradient(circle at 50% 20%, rgba(239, 68, 68, 0.2), transparent 70%), var(--card-bg); }
.status-hero.lost .status-pill { background: rgba(239, 68, 68, 0.2); color: var(--accent-red); border: 1px solid var(--accent-red); }
.status-hero.loading { border-color: rgba(59, 130, 246, 0.4); background: var(--card-bg); }
.status-hero.loading .status-pill { background: rgba(59, 130, 246, 0.2); color: var(--primary); border: 1px solid var(--primary); }

.status-title { font-size: 1.85rem; font-weight: 800; display: flex; align-items: center; gap: 10px; }
.status-timer { font-size: 1.1rem; color: var(--text-muted); font-variant-numeric: tabular-nums; }
.alarm-countdown { margin-top: 6px; font-size: 0.95rem; color: var(--accent-yellow); font-weight: 600; padding: 6px 12px; background: rgba(245, 158, 11, 0.12); border-radius: 8px; border: 1px dashed var(--accent-yellow); }

.grid-stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap: 10px; margin-top: 15px; width: 100%; }
.stat-box { background: rgba(0, 0, 0, 0.3); border: 1px solid var(--card-border); border-radius: 10px; padding: 10px; text-align: center; }
.stat-val { font-size: 1rem; font-weight: 700; margin-top: 4px; color: #fff; }
.stat-lbl { font-size: 0.7rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.5px; }

.section-title { font-size: 1.05rem; font-weight: 700; margin-bottom: 14px; display: flex; align-items: center; gap: 8px; color: #fff; }
.form-group { margin-bottom: 14px; }
.form-row { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; }
@media (max-width: 600px) { .form-row { grid-template-columns: 1fr; } }
label { display: block; font-size: 0.82rem; font-weight: 600; color: var(--text-muted); margin-bottom: 5px; }
input[type="text"], input[type="password"], input[type="number"], textarea, select {
  width: 100%; background: var(--input-bg); border: 1px solid var(--input-border); color: #fff; padding: 9px 12px; border-radius: 8px; font-size: 0.88rem; transition: 0.2s;
}
input:focus, textarea:focus, select:focus { outline: none; border-color: var(--primary); box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.25); }
textarea { min-height: 65px; resize: vertical; }
.checkbox-label { display: flex; align-items: center; gap: 8px; cursor: pointer; color: var(--text-main); font-size: 0.88rem; margin-top: 6px; }
.checkbox-label input { width: 18px; height: 18px; accent-color: var(--primary); }

.btn { display: inline-flex; align-items: center; justify-content: center; gap: 8px; padding: 9px 16px; border-radius: 8px; font-size: 0.88rem; font-weight: 600; cursor: pointer; border: none; transition: 0.2s; }
.btn-primary { background: var(--primary); color: white; }
.btn-primary:hover { background: var(--primary-hover); }
.btn-secondary { background: rgba(255, 255, 255, 0.08); color: var(--text-main); border: 1px solid var(--card-border); }
.btn-secondary:hover { background: rgba(255, 255, 255, 0.15); }
.btn-danger { background: rgba(239, 68, 68, 0.15); color: var(--accent-red); border: 1px solid rgba(239, 68, 68, 0.3); }
.btn-danger:hover { background: rgba(239, 68, 68, 0.25); }
.btn-warning { background: rgba(245, 158, 11, 0.15); color: var(--accent-yellow); border: 1px solid rgba(245, 158, 11, 0.3); }
.actions-row { display: flex; gap: 10px; flex-wrap: wrap; margin-top: 15px; }

.log-box { background: #000; border: 1px solid var(--card-border); border-radius: 8px; padding: 12px; font-family: monospace; font-size: 0.8rem; max-height: 200px; overflow-y: auto; display: flex; flex-direction: column-reverse; gap: 4px; }
.log-item { display: flex; gap: 8px; line-height: 1.4; }
.log-time { color: var(--text-muted); }
.log-item.ok .log-text { color: #34d399; }
.log-item.err .log-text { color: #f87171; }

.tab-pane { display: none; }
.tab-pane.active { display: block; }
.alert-toast { position: fixed; bottom: 20px; right: 20px; padding: 12px 20px; border-radius: 8px; background: #1e293b; color: white; border: 1px solid #334155; box-shadow: 0 10px 25px rgba(0,0,0,0.5); z-index: 1000; display: none; }
</style>
</head>
<body>
<div class="container">
  <header>
    <div class="brand">
      <div class="brand-icon">⚡</div>
      <div>
        <h1>ESP32 Монітор Напруги</h1>
        <span id="headerSub">ESP32 DevKit V1 (ESP-WROOM-32)</span>
      </div>
    </div>
    <div class="nav-tabs">
      <button class="tab-btn active" id="btnDash" onclick="showTab('dashTab', this)">Статус</button>
      <button class="tab-btn" id="btnSettings" onclick="showTab('settingsTab', this)">Налаштування</button>
      <button class="tab-btn" id="btnOta" onclick="showTab('otaTab', this)">Оновлення (OTA)</button>
    </div>
  </header>

  <!-- TAB: DASHBOARD -->
  <div id="dashTab" class="tab-pane active">
    <div id="heroCard" class="card status-hero loading">
      <div id="statusBadge" class="status-pill">⏳ Отримання даних...</div>
      <div id="statusTitle" class="status-title">⏳ Синхронізація...</div>
      <div id="statusTimer" class="status-timer">Опитування датчика GPIO 4...</div>
      <div id="alarmCountdown" class="alarm-countdown" style="display:none;"></div>

      <div class="grid-stats">
        <div class="stat-box">
          <div class="stat-lbl">Вхід GPIO 4</div>
          <div id="statGpio" class="stat-val">...</div>
        </div>
        <div class="stat-box">
          <div class="stat-lbl">Wi-Fi Мережа</div>
          <div id="statSsid" class="stat-val">...</div>
        </div>
        <div class="stat-box">
          <div class="stat-lbl">IP Адреса</div>
          <div id="statIp" class="stat-val">...</div>
        </div>
        <div class="stat-box">
          <div class="stat-lbl">Сигнал RSSI</div>
          <div id="statRssi" class="stat-val">...</div>
        </div>
        <div class="stat-box">
          <div class="stat-lbl">ESP32 Uptime</div>
          <div id="statUptime" class="stat-val">...</div>
        </div>
        <div class="stat-box">
          <div class="stat-lbl">Free Heap</div>
          <div id="statHeap" class="stat-val">...</div>
        </div>
      </div>
    </div>

    <div class="card" style="margin-top: 15px;">
      <div class="section-title">⚡ Керування та Тести</div>
      <div class="actions-row">
        <button class="btn btn-primary" onclick="triggerTest()">🔔 Надіслати тестовий алерт (Signal + Google)</button>
        <button class="btn btn-secondary" onclick="rebootDevice()">🔄 Перезавантажити ESP32</button>
        <button class="btn btn-warning" onclick="fetchStatus()">🔄 Оновити дані</button>
      </div>
    </div>

    <div class="card" style="margin-top: 15px;">
      <div class="section-title">📜 Журнал сповіщень та подій</div>
      <div id="logBox" class="log-box">
        <div class="log-item"><span class="log-time">--:--:--</span> <span class="log-text">Очікування подій...</span></div>
      </div>
    </div>
  </div>

  <!-- TAB: SETTINGS -->
  <div id="settingsTab" class="tab-pane">
    <div class="card">
      <form id="cfgForm" onsubmit="saveConfig(event)">
        <div class="section-title">📶 Налаштування Wi-Fi та Мережі</div>
        <div class="form-row">
          <div class="form-group">
            <label>Wi-Fi SSID (Мережа)</label>
            <input type="text" id="wifi_ssid" required placeholder="Назва роутера">
          </div>
          <div class="form-group">
            <label>Wi-Fi Пароль</label>
            <input type="password" id="wifi_password" placeholder="Залиште пустим, якщо відкрита">
          </div>
        </div>
        <div class="form-row">
          <div class="form-group">
            <label>Ім'я точки доступу (SoftAP SSID)</label>
            <input type="text" id="ap_ssid" placeholder="PowerMonitor-Setup">
          </div>
          <div class="form-group">
            <label>Пароль точки доступу AP (мін. 8 символів або порожньо)</label>
            <input type="password" id="ap_password" placeholder="Порожньо = відкрита">
          </div>
        </div>
        <div class="form-group">
          <label>Локальне доменне ім'я (mDNS Hostname)</label>
          <input type="text" id="hostname" placeholder="power-monitor">
          <small style="color:var(--text-muted);font-size:0.75rem;">Буде доступно за адресою: http://&lt;hostname&gt;.local</small>
        </div>

        <div class="section-title" style="margin-top: 25px;">💬 Signal REST API Інтеграція</div>
        <div class="form-row">
          <div class="form-group">
            <label>Адреса сервера Signal REST API</label>
            <input type="text" id="signal_url" placeholder="http://192.168.1.100:8080">
          </div>
          <div class="form-group">
            <label>Номер-відправник у Signal (Registered Number)</label>
            <input type="text" id="signal_sender" placeholder="+380XXXXXXXXX">
          </div>
        </div>

        <div class="section-title" style="margin-top: 25px;">📊 Google Sheets Webhook (Apps Script)</div>
        <div class="form-group">
          <label>Google Apps Script Webhook URL</label>
          <input type="text" id="google_webhook_url" placeholder="https://script.google.com/macros/s/.../exec">
        </div>

        <div class="section-title" style="margin-top: 25px;">👥 Одержувачі сповіщень</div>
        <div class="form-row">
          <div class="form-group">
            <label>Основний номер телефону</label>
            <input type="text" id="primary_phone" placeholder="+380671112233">
          </div>
          <div class="form-group">
            <label>Резервні номери (розділені комою)</label>
            <input type="text" id="backup_phones_csv" placeholder="+380501112233, +380931112233">
          </div>
        </div>

        <div class="section-title" style="margin-top: 25px;">✉️ Матриця шаблонів повідомлень Signal</div>
        <div class="form-row">
          <div class="form-group">
            <label>Подія T = 0 (Зникнення 5V): Текст для основного номера</label>
            <textarea id="msg_t0_primary"></textarea>
          </div>
          <div class="form-group">
            <label>Подія T = 0 (Зникнення 5V): Текст для резервних номерів</label>
            <textarea id="msg_t0_backup"></textarea>
          </div>
        </div>
        <div class="form-row">
          <div class="form-group">
            <label>Подія T = 10 хв (Тривала відсутність): Текст для основного</label>
            <textarea id="msg_t10_primary"></textarea>
          </div>
          <div class="form-group">
            <label>Подія T = 10 хв (Тривала відсутність): Текст для резервних</label>
            <textarea id="msg_t10_backup"></textarea>
          </div>
        </div>
        <div class="form-row">
          <div class="form-group">
            <label>Подія Відновлення живлення: Текст для основного номера</label>
            <textarea id="msg_restored_primary"></textarea>
          </div>
          <div class="form-group">
            <label>Подія Відновлення живлення: Текст для резервних</label>
            <textarea id="msg_restored_backup"></textarea>
          </div>
        </div>

        <div class="section-title" style="margin-top: 25px;">⚙️ Апаратні налаштування (ESP32 DevKit V1)</div>
        <div class="form-row">
          <div class="form-group">
            <label>GPIO вхід контролю 5V</label>
            <input type="number" id="sense_gpio" min="0" max="48" value="4">
          </div>
          <div class="form-group">
            <label>Фільтр брязкоту (Debounce, мс)</label>
            <input type="number" id="debounce_ms" min="100" max="10000" value="2000">
          </div>
        </div>
        <div class="form-row">
          <div class="form-group">
            <label>Таймаут тривоги T2 (секунди, 600 = 10 хв)</label>
            <input type="number" id="alarm_timeout_sec" min="10" max="86400" value="600">
          </div>
          <div class="form-group" style="display:flex;align-items:center;">
            <label class="checkbox-label">
              <input type="checkbox" id="sense_inverted">
              Інвертований вхід (LOW = напруга є, для оптопари)
            </label>
          </div>
        </div>

        <div class="actions-row" style="margin-top: 20px;">
          <button type="submit" class="btn btn-primary">💾 Зберегти конфігурацію</button>
          <button type="button" class="btn btn-danger" onclick="factoryReset()">⚠️ Скинути до заводських налаштувань</button>
        </div>
      </form>
    </div>
  </div>

  <!-- TAB: OTA UPDATE -->
  <div id="otaTab" class="tab-pane">
    <div class="card">
      <div class="section-title">🚀 Оновлення прошивки по повітрю (OTA)</div>
      <p style="color:var(--text-muted);font-size:0.9rem;margin-bottom:15px;">
        Оберіть скомпільований бінарний файл прошивки (<code>firmware.bin</code>) для ESP32 DevKit V1 (ESP-WROOM-32). Оновлення пройде безпечно через подвійний слот OTA.
      </p>
      <form method="POST" action="/update" enctype="multipart/form-data" id="otaForm" onsubmit="handleOta(event)">
        <div class="form-group">
          <label>Файл прошивки (.bin)</label>
          <input type="file" name="update" id="otaFile" accept=".bin" required>
        </div>
        <div id="otaProgress" style="display:none;margin-bottom:15px;">
          <label id="otaStatusText">Завантаження прошивки...</label>
          <div style="background:#111827;height:12px;border-radius:6px;overflow:hidden;margin-top:6px;">
            <div id="otaBar" style="background:var(--primary);height:100%;width:0%;transition:width 0.2s;"></div>
          </div>
        </div>
        <button type="submit" id="otaSubmitBtn" class="btn btn-primary">⚡ Почати прошивку</button>
      </form>
    </div>
  </div>
</div>

<div id="toast" class="alert-toast"></div>

<script>
function showTab(tabId, btn) {
  var panes = document.getElementsByClassName("tab-pane");
  for (var i = 0; i < panes.length; i++) {
    panes[i].classList.remove("active");
  }
  var buttons = document.getElementsByClassName("tab-btn");
  for (var j = 0; j < buttons.length; j++) {
    buttons[j].classList.remove("active");
  }
  var targetTab = document.getElementById(tabId);
  if (targetTab) targetTab.classList.add("active");
  if (btn) {
    btn.classList.add("active");
  } else {
    if (tabId === "dashTab") document.getElementById("btnDash")?.classList.add("active");
    if (tabId === "settingsTab") document.getElementById("btnSettings")?.classList.add("active");
    if (tabId === "otaTab") document.getElementById("btnOta")?.classList.add("active");
  }
}

function showToast(msg, duration) {
  var d = duration || 3500;
  var t = document.getElementById("toast");
  if (!t) return;
  t.innerText = msg;
  t.style.display = "block";
  setTimeout(function() { t.style.display = "none"; }, d);
}

function formatSec(sec) {
  var s = Number(sec) || 0;
  var h = Math.floor(s / 3600);
  var m = Math.floor((s % 3600) / 60);
  var remSec = s % 60;
  return (h < 10 ? "0" + h : h) + ":" + (m < 10 ? "0" + m : m) + ":" + (remSec < 10 ? "0" + remSec : remSec);
}

async function fetchStatus() {
  try {
    const res = await fetch("/api/status");
    if (!res.ok) {
      console.warn("fetchStatus HTTP " + res.status);
      return;
    }
    const d = await res.json();
    console.log("Telemetry updated:", d);

    const hero = document.getElementById("heroCard");
    const badge = document.getElementById("statusBadge");
    const title = document.getElementById("statusTitle");
    const timer = document.getElementById("statusTimer");
    const alarmEl = document.getElementById("alarmCountdown");

    if (d.power_present) {
      if (hero) hero.className = "card status-hero ok";
      if (badge) badge.innerText = "Мережа Активна (220V/5V є)";
      if (title) title.innerText = "⚡ Напруга 5V присутня";
      if (timer) timer.innerText = "Світло є вже: " + formatSec(d.duration_sec || 0);
      if (alarmEl) alarmEl.style.display = "none";
    } else {
      if (hero) hero.className = "card status-hero lost";
      if (badge) badge.innerText = "Знеструмлено (5V зникло)";
      if (title) title.innerText = "⚠️ Напруга ВІДСУТНЯ";
      if (timer) timer.innerText = "Світла немає вже: " + formatSec(d.duration_sec || 0);

      if (alarmEl) {
        if (d.countdown_sec > 0) {
          alarmEl.style.display = "block";
          alarmEl.innerText = "⏳ До сповіщення Т=10 хв залишилося: " + formatSec(d.countdown_sec);
        } else {
          alarmEl.style.display = "block";
          alarmEl.innerText = "🚨 Сповіщення тривалої відсутності (Т=10 хв) НАДІСЛАНО!";
        }
      }
    }

    const gpioVal = (d.raw_gpio === 1) ? "HIGH (5V)" : "LOW (0V / відкл.)";
    const elGpio = document.getElementById("statGpio");
    if (elGpio) elGpio.innerText = gpioVal;

    const elSsid = document.getElementById("statSsid");
    if (elSsid) elSsid.innerText = d.ssid || "---";

    const elIp = document.getElementById("statIp");
    if (elIp) elIp.innerText = d.ip || "0.0.0.0";

    const elRssi = document.getElementById("statRssi");
    if (elRssi) elRssi.innerText = (d.rssi || 0) + " dBm";

    const elUptime = document.getElementById("statUptime");
    if (elUptime) elUptime.innerText = formatSec(d.uptime_sec || 0);

    const elHeap = document.getElementById("statHeap");
    if (elHeap) elHeap.innerText = Math.round((d.free_heap || 0) / 1024) + " KB";

    if (d.logs && Array.isArray(d.logs)) {
      const logBox = document.getElementById("logBox");
      if (logBox) {
        if (d.logs.length === 0) {
          logBox.innerHTML = '<div class="log-item"><span class="log-time">--:--:--</span> <span class="log-text">Немає записів у журналі (Очікування подій)</span></div>';
        } else {
          logBox.innerHTML = "";
          d.logs.forEach(l => {
            const div = document.createElement("div");
            div.className = "log-item " + (l.success ? "ok" : "err");
            div.innerHTML = `<span class="log-time">[${l.time}]</span> <span class="log-text">${l.msg}</span>`;
            logBox.appendChild(div);
          });
        }
      }
    }
  } catch (e) {
    console.error("Status fetch error", e);
  }
}

async function loadConfig() {
  try {
    const res = await fetch("/api/config");
    if (!res.ok) return;
    const cfg = await res.json();

    const setVal = (id, val) => { const el = document.getElementById(id); if (el) el.value = val; };
    const setChk = (id, val) => { const el = document.getElementById(id); if (el) el.checked = val; };

    setVal("wifi_ssid", cfg.wifi_ssid || "");
    setVal("wifi_password", cfg.wifi_password || "");
    setVal("ap_ssid", cfg.ap_ssid || "");
    setVal("ap_password", cfg.ap_password || "");
    setVal("hostname", cfg.hostname || "");

    setVal("signal_url", cfg.signal_url || "");
    setVal("signal_sender", cfg.signal_sender || "");
    setVal("google_webhook_url", cfg.google_webhook_url || "");

    setVal("primary_phone", cfg.primary_phone || "");
    setVal("backup_phones_csv", cfg.backup_phones_csv || "");

    setVal("msg_t0_primary", cfg.msg_t0_primary || "");
    setVal("msg_t0_backup", cfg.msg_t0_backup || "");
    setVal("msg_t10_primary", cfg.msg_t10_primary || "");
    setVal("msg_t10_backup", cfg.msg_t10_backup || "");
    setVal("msg_restored_primary", cfg.msg_restored_primary || "");
    setVal("msg_restored_backup", cfg.msg_restored_backup || "");

    setVal("sense_gpio", cfg.sense_gpio ?? 4);
    setChk("sense_inverted", !!cfg.sense_inverted);
    setVal("debounce_ms", cfg.debounce_ms ?? 2000);
    setVal("alarm_timeout_sec", cfg.alarm_timeout_sec ?? 600);
  } catch (e) {
    console.error("Config load error", e);
  }
}

async function saveConfig(e) {
  e.preventDefault();
  const cfg = {
    wifi_ssid: document.getElementById("wifi_ssid").value,
    wifi_password: document.getElementById("wifi_password").value,
    ap_ssid: document.getElementById("ap_ssid").value,
    ap_password: document.getElementById("ap_password").value,
    hostname: document.getElementById("hostname").value,

    signal_url: document.getElementById("signal_url").value,
    signal_sender: document.getElementById("signal_sender").value,
    google_webhook_url: document.getElementById("google_webhook_url").value,

    primary_phone: document.getElementById("primary_phone").value,
    backup_phones_csv: document.getElementById("backup_phones_csv").value,

    msg_t0_primary: document.getElementById("msg_t0_primary").value,
    msg_t0_backup: document.getElementById("msg_t0_backup").value,
    msg_t10_primary: document.getElementById("msg_t10_primary").value,
    msg_t10_backup: document.getElementById("msg_t10_backup").value,
    msg_restored_primary: document.getElementById("msg_restored_primary").value,
    msg_restored_backup: document.getElementById("msg_restored_backup").value,

    sense_gpio: parseInt(document.getElementById("sense_gpio").value),
    sense_inverted: document.getElementById("sense_inverted").checked,
    debounce_ms: parseInt(document.getElementById("debounce_ms").value),
    alarm_timeout_sec: parseInt(document.getElementById("alarm_timeout_sec").value)
  };

  try {
    const res = await fetch("/api/config", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(cfg)
    });
    if (res.ok) {
      showToast("✅ Налаштування успішно збережено!");
      fetchStatus();
    } else {
      showToast("❌ Помилка збереження налаштувань");
    }
  } catch (e) {
    showToast("❌ Помилка з'єднання");
  }
}

async function triggerTest() {
  try {
    const res = await fetch("/api/test", { method: "POST" });
    if (res.ok) {
      showToast("🚀 Тестовий алерт надіслано в чергу!");
      setTimeout(fetchStatus, 1500);
    } else {
      showToast("❌ Помилка відправки тесту");
    }
  } catch (e) {
    showToast("❌ Помилка зв'язку");
  }
}

async function rebootDevice() {
  if (!confirm("Перезавантажити ESP32?")) return;
  try {
    await fetch("/api/reboot", { method: "POST" });
    showToast("🔄 Пристрій перезавантажується... Будь ласка, зачекайте 10 секунд");
    setTimeout(function() { location.reload(); }, 10000);
  } catch (e) {
    showToast("🔄 Запит на перезавантаження надіслано");
  }
}

async function factoryReset() {
  if (!confirm("Ви впевнені, що хочете скинути ВСІ налаштування до заводських? Пристрій перезапуститься в режим точки доступу!")) return;
  try {
    await fetch("/api/factory-reset", { method: "POST" });
    showToast("⚠️ Налаштування скинуто. Перезавантаження...");
    setTimeout(function() { location.reload(); }, 8000);
  } catch (e) {
    showToast("Помилка скидання");
  }
}

function handleOta(e) {
  e.preventDefault();
  const fileInput = document.getElementById("otaFile");
  if (!fileInput.files.length) return;

  const file = fileInput.files[0];
  const formData = new FormData();
  formData.append("update", file);

  const progBox = document.getElementById("otaProgress");
  const bar = document.getElementById("otaBar");
  const statusTxt = document.getElementById("otaStatusText");
  const btn = document.getElementById("otaSubmitBtn");

  progBox.style.display = "block";
  btn.disabled = true;

  const xhr = new XMLHttpRequest();
  xhr.open("POST", "/update");

  xhr.upload.onprogress = function(event) {
    if (event.lengthComputable) {
      const pct = Math.round((event.loaded / event.total) * 100);
      bar.style.width = pct + "%";
      statusTxt.innerText = "Завантаження прошивки: " + pct + "%";
    }
  };

  xhr.onload = function() {
    if (xhr.status === 200) {
      statusTxt.innerText = "✅ Прошивка успішно оновлена! Перезавантаження...";
      bar.style.background = "#10b981";
      setTimeout(function() { location.reload(); }, 12000);
    } else {
      statusTxt.innerText = "❌ Помилка оновлення: " + xhr.responseText;
      bar.style.background = "#ef4444";
      btn.disabled = false;
    }
  };

  xhr.onerror = function() {
    statusTxt.innerText = "❌ Збій передачі даних по мережі";
    bar.style.background = "#ef4444";
    btn.disabled = false;
  };

  xhr.send(formData);
}

// Initial execution
fetchStatus();
setTimeout(loadConfig, 250);
setInterval(fetchStatus, 2500);
</script>
</body>
</html>
"""

compressed = gzip.compress(html.encode('utf-8'), compresslevel=9)
print(f"Original HTML: {len(html)} bytes")
print(f"Gzipped HTML:  {len(compressed)} bytes")

# Generate web_assets.h
header_content = f"""#pragma once

#include <Arduino.h>

const size_t INDEX_HTML_GZ_LEN = {len(compressed)};
const uint8_t INDEX_HTML_GZ[] PROGMEM = {{
"""

bytes_per_line = 16
for i in range(0, len(compressed), bytes_per_line):
    chunk = compressed[i:i+bytes_per_line]
    hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
    header_content += f"    {hex_str},\n"

header_content += "};\n"

with open("include/web_assets.h", "w", encoding="utf-8") as f:
    f.write(header_content)

print("Generated include/web_assets.h successfully!")
