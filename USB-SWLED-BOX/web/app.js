(() => {
  "use strict";

  const BAUD_RATE = 115200;
  const POLL_INTERVAL_MS = 100;
  const BLINK_INTERVAL_MS = 500;

  let port = null;
  let reader = null;
  let writer = null;
  let readLoopActive = false;
  let disconnecting = false;

  let pollTimer = null;
  let greenBlinkTimer = null;
  let redBlinkTimer = null;

  let greenBlinkState = false;
  let redBlinkState = false;
  let lastSwitchState = null;
  let receiveBuffer = "";

  const encoder = new TextEncoder();
  const decoder = new TextDecoder();

  const $ = (id) => document.getElementById(id);

  const el = {
    connectBtn: $("connectBtn"),
    disconnectBtn: $("disconnectBtn"),
    checkSwitchBtn: $("checkSwitchBtn"),
    greenOnBtn: $("greenOnBtn"),
    greenOffBtn: $("greenOffBtn"),
    greenBlinkBtn: $("greenBlinkBtn"),
    redOnBtn: $("redOnBtn"),
    redOffBtn: $("redOffBtn"),
    redBlinkBtn: $("redBlinkBtn"),
    allOnBtn: $("allOnBtn"),
    allOffBtn: $("allOffBtn"),
    clearLogBtn: $("clearLogBtn"),
    downloadLogBtn: $("downloadLogBtn"),
    connectionDot: $("connectionDot"),
    connectionLabel: $("connectionLabel"),
    connectionDetail: $("connectionDetail"),
    switchIndicator: $("switchIndicator"),
    switchText: $("switchText"),
    pollStatus: $("pollStatus"),
    supportMessage: $("supportMessage"),
    portName: $("portName"),
    lastRx: $("lastRx"),
    greenState: $("greenState"),
    redState: $("redState"),
    log: $("log")
  };

  function timestamp() {
    const d = new Date();
    const hms = d.toLocaleTimeString("ja-JP", { hour12: false });
    const ms = String(d.getMilliseconds()).padStart(3, "0");
    return `${hms}.${ms}`;
  }

  function log(message) {
    el.log.textContent += `${timestamp()}  ${message}\n`;
    el.log.scrollTop = el.log.scrollHeight;
  }

  function setConnectedUI(connected) {
    const controls = [
      el.checkSwitchBtn,
      el.greenOnBtn, el.greenOffBtn, el.greenBlinkBtn,
      el.redOnBtn, el.redOffBtn, el.redBlinkBtn,
      el.allOnBtn, el.allOffBtn
    ];

    el.connectBtn.disabled = connected;
    el.disconnectBtn.disabled = !connected;
    controls.forEach((button) => button.disabled = !connected);

    if (connected) {
      el.connectionDot.className = "status-dot online";
      el.connectionLabel.textContent = "接続中";
      el.connectionDetail.textContent = "USB-SWLED BOX 通信中";
      el.pollStatus.textContent = "監視中";
      el.pollStatus.className = "mini-status live";
    } else {
      el.connectionDot.className = "status-dot offline";
      el.connectionLabel.textContent = "未接続";
      el.connectionDetail.textContent = "USB-SWLED BOXを接続してください";
      el.pollStatus.textContent = "停止";
      el.pollStatus.className = "mini-status idle";
      el.portName.textContent = "---";
      el.lastRx.textContent = "---";
      el.switchText.textContent = "---";
      el.switchIndicator.className = "switch-display unknown";
      lastSwitchState = null;
      el.greenState.textContent = "停止";
      el.redState.textContent = "停止";
    }
  }

  function setSwitchState(isOn) {
    el.switchText.textContent = isOn ? "ON" : "OFF";
    el.switchIndicator.className = "switch-display " + (isOn ? "on" : "off");
    el.lastRx.textContent = timestamp();

    if (lastSwitchState !== isOn) {
      log(`SWITCH = ${isOn ? "ON" : "OFF"}`);
      lastSwitchState = isOn;
    }
  }

  function getPortLabel() {
    if (!port) return "---";
    const info = typeof port.getInfo === "function" ? port.getInfo() : {};
    const usbVendorId = info.usbVendorId ? `VID:${info.usbVendorId.toString(16).toUpperCase().padStart(4, "0")}` : "";
    const usbProductId = info.usbProductId ? `PID:${info.usbProductId.toString(16).toUpperCase().padStart(4, "0")}` : "";
    return [usbVendorId, usbProductId].filter(Boolean).join(" ") || "Serial Port";
  }

  async function sendRaw(command, writeLog = true) {
    if (!writer) return;

    try {
      await writer.write(encoder.encode(command + "\r\n"));
      if (writeLog) log(`TX > ${command}`);
    } catch (err) {
      log(`送信エラー: ${err?.message || err}`);
      if (!disconnecting) await disconnect();
    }
  }

  function handleLine(line) {
    const data = line.trim();
    if (!data) return;

    el.lastRx.textContent = timestamp();

    switch (data) {
      case "i11":
        setSwitchState(true);
        break;
      case "i10":
        setSwitchState(false);
        break;
      case "o1":
        log("RX < o1  緑LED 設定完了");
        break;
      case "o2":
        log("RX < o2  赤LED 設定完了");
        break;
      default:
        log(`RX < ${data}`);
        break;
    }
  }

  function consumeReceivedText(text) {
    receiveBuffer += text;

    while (true) {
      const idx = receiveBuffer.indexOf("\r\n");
      if (idx < 0) break;

      const line = receiveBuffer.slice(0, idx);
      receiveBuffer = receiveBuffer.slice(idx + 2);
      handleLine(line);
    }
  }

  async function readLoop() {
    readLoopActive = true;

    try {
      while (port && port.readable && readLoopActive) {
        reader = port.readable.getReader();

        try {
          while (readLoopActive) {
            const { value, done } = await reader.read();
            if (done) break;
            if (value) consumeReceivedText(decoder.decode(value, { stream: true }));
          }
        } finally {
          if (reader) {
            reader.releaseLock();
            reader = null;
          }
        }
      }
    } catch (err) {
      if (readLoopActive && !disconnecting) {
        log(`受信エラー: ${err?.message || err}`);
        await disconnect();
      }
    }
  }

  function startPolling() {
    stopPolling();
    pollTimer = setInterval(() => sendRaw("i1", false), POLL_INTERVAL_MS);
  }

  function stopPolling() {
    if (pollTimer) {
      clearInterval(pollTimer);
      pollTimer = null;
    }
  }

  function stopGreenBlink(sendOff = false) {
    if (greenBlinkTimer) {
      clearInterval(greenBlinkTimer);
      greenBlinkTimer = null;
    }
    greenBlinkState = false;
    el.greenBlinkBtn.textContent = "点滅 ON";
    el.greenState.textContent = "停止";
    if (sendOff && writer) sendRaw("o10");
  }

  function stopRedBlink(sendOff = false) {
    if (redBlinkTimer) {
      clearInterval(redBlinkTimer);
      redBlinkTimer = null;
    }
    redBlinkState = false;
    el.redBlinkBtn.textContent = "点滅 ON";
    el.redState.textContent = "停止";
    if (sendOff && writer) sendRaw("o20");
  }

  function toggleGreenBlink() {
    if (greenBlinkTimer) {
      stopGreenBlink(true);
      log("緑LED 点滅停止");
      return;
    }

    greenBlinkState = false;
    el.greenBlinkBtn.textContent = "点滅 OFF";
    el.greenState.textContent = "点滅中（500 ms）";
    log("緑LED 点滅開始");

    greenBlinkTimer = setInterval(() => {
      greenBlinkState = !greenBlinkState;
      sendRaw(greenBlinkState ? "o11" : "o10");
    }, BLINK_INTERVAL_MS);
  }

  function toggleRedBlink() {
    if (redBlinkTimer) {
      stopRedBlink(true);
      log("赤LED 点滅停止");
      return;
    }

    redBlinkState = false;
    el.redBlinkBtn.textContent = "点滅 OFF";
    el.redState.textContent = "点滅中（500 ms）";
    log("赤LED 点滅開始");

    redBlinkTimer = setInterval(() => {
      redBlinkState = !redBlinkState;
      sendRaw(redBlinkState ? "o21" : "o20");
    }, BLINK_INTERVAL_MS);
  }

  async function connect() {
    if (!("serial" in navigator)) {
      el.supportMessage.textContent = "Web Serial API非対応です。Windows版のChromeまたはEdgeを使用してください。";
      return;
    }

    try {
      port = await navigator.serial.requestPort();

      await port.open({
        baudRate: BAUD_RATE,
        dataBits: 8,
        stopBits: 1,
        parity: "none",
        flowControl: "none"
      });

      writer = port.writable.getWriter();
      receiveBuffer = "";

      setConnectedUI(true);
      el.portName.textContent = getPortLabel();

      log("USB-SWLED BOX 接続");
      log("115200bps / 8N1 / CRLF");
      log("スイッチ状態リアルタイム監視開始");

      readLoop();
      startPolling();
      await sendRaw("i1", false);
    } catch (err) {
      if (err?.name === "NotFoundError") {
        log("接続キャンセル");
      } else {
        log(`接続エラー: ${err?.message || err}`);
      }

      if (port && !writer) {
        try { await port.close(); } catch {}
        port = null;
      }
      setConnectedUI(false);
    }
  }

  async function disconnect() {
    if (disconnecting) return;
    disconnecting = true;

    stopPolling();
    stopGreenBlink(false);
    stopRedBlink(false);
    readLoopActive = false;

    if (reader) {
      try { await reader.cancel(); } catch {}
    }

    if (writer) {
      try { writer.releaseLock(); } catch {}
      writer = null;
    }

    if (port) {
      try { await port.close(); } catch {}
      port = null;
    }

    setConnectedUI(false);
    log("切断");
    disconnecting = false;
  }

  function downloadLog() {
    const content = el.log.textContent || "";
    if (!content.trim()) {
      log("保存するログがありません");
      return;
    }

    const blob = new Blob([content], { type: "text/plain;charset=utf-8" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    const d = new Date();
    const stamp = [
      d.getFullYear(),
      String(d.getMonth() + 1).padStart(2, "0"),
      String(d.getDate()).padStart(2, "0"),
      "_",
      String(d.getHours()).padStart(2, "0"),
      String(d.getMinutes()).padStart(2, "0"),
      String(d.getSeconds()).padStart(2, "0")
    ].join("");

    a.href = url;
    a.download = `USB_SWLED_BOX_${stamp}.txt`;
    document.body.appendChild(a);
    a.click();
    a.remove();
    URL.revokeObjectURL(url);
  }

  el.connectBtn.addEventListener("click", connect);
  el.disconnectBtn.addEventListener("click", disconnect);
  el.checkSwitchBtn.addEventListener("click", () => sendRaw("i1"));

  el.greenOnBtn.addEventListener("click", () => {
    stopGreenBlink(false);
    el.greenState.textContent = "ON";
    sendRaw("o11");
  });

  el.greenOffBtn.addEventListener("click", () => {
    stopGreenBlink(false);
    el.greenState.textContent = "OFF";
    sendRaw("o10");
  });

  el.greenBlinkBtn.addEventListener("click", toggleGreenBlink);

  el.redOnBtn.addEventListener("click", () => {
    stopRedBlink(false);
    el.redState.textContent = "ON";
    sendRaw("o21");
  });

  el.redOffBtn.addEventListener("click", () => {
    stopRedBlink(false);
    el.redState.textContent = "OFF";
    sendRaw("o20");
  });

  el.redBlinkBtn.addEventListener("click", toggleRedBlink);

  el.allOnBtn.addEventListener("click", () => {
    stopGreenBlink(false);
    stopRedBlink(false);
    el.greenState.textContent = "ON";
    el.redState.textContent = "ON";
    sendRaw("o11");
    sendRaw("o21");
  });

  el.allOffBtn.addEventListener("click", () => {
    stopGreenBlink(false);
    stopRedBlink(false);
    el.greenState.textContent = "OFF";
    el.redState.textContent = "OFF";
    sendRaw("o10");
    sendRaw("o20");
  });

  el.clearLogBtn.addEventListener("click", () => {
    el.log.textContent = "";
  });

  el.downloadLogBtn.addEventListener("click", downloadLog);

  if ("serial" in navigator) {
    el.supportMessage.textContent = "Chrome / Edgeで使用できます。接続ボタンを押してUSB-SWLED BOXのシリアルポートを選択してください。";
  } else {
    el.supportMessage.textContent = "Web Serial API非対応ブラウザです。Windows版ChromeまたはEdgeを使用してください。";
    el.connectBtn.disabled = true;
  }

  if ("serial" in navigator && typeof navigator.serial.addEventListener === "function") {
    navigator.serial.addEventListener("disconnect", (event) => {
      if (port && event.target === port) {
        log("USB-SWLED BOX が取り外されました");
        disconnect();
      }
    });
  }

  window.addEventListener("beforeunload", () => {
    stopPolling();
    stopGreenBlink(false);
    stopRedBlink(false);
  });

  setConnectedUI(false);
})();
