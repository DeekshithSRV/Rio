// ================= MQTT =================
const client = mqtt.connect(
  "wss://test.mosquitto.org:8081/mqtt"
);

// ================= VARIABLES =================
let deviceID = null;
let deviceOnline = false;

let statusTopic = "";
let ackTopic = "";

let lightTopic = "";
let fanTopic = "";
let pumpTopic = "";

let wifiSSIDTopic = "";
let wifiPasswordTopic = "";
let wifiSaveTopic = "";

let tankLevel = 0;
let pumpRunning = false;

let logs = [];

// ================= TERMINAL =================

function getTime() {
  return new Date()
    .toLocaleTimeString();
}

function terminal(msg) {

  const box =
    document.getElementById(
      "terminal"
    );

  box.innerHTML += `
    <div>
      ${getTime()}
      →
      ${msg}
    </div>
  `;

  box.scrollTop =
    box.scrollHeight;
}

// ================= ACTIVITY LOGS =================

function addLog(
  device,
  action,
  status
) {

  const table =
    document.getElementById(
      "logTable"
    );

  const row =
    table.insertRow(0);

  row.insertCell(0)
    .innerHTML =
    getTime();

  row.insertCell(1)
    .innerHTML =
    device;

  row.insertCell(2)
    .innerHTML =
    action;

  row.insertCell(3)
    .innerHTML =
    status;

  logs.push({
    time: getTime(),
    device,
    action,
    status
  });
}

// ================= CHART =================

const ctx =
  document.getElementById(
    "waterChart"
  );

const waterChart =
  new Chart(
    ctx,
    {
      type: "line",

      data: {
        labels: [],

        datasets: [
          {
            label:
              "Tank Level %",
            data: [],
            borderColor:
              "#2196f3",
            backgroundColor:
              "rgba(33,150,243,0.2)",
            fill: true,
            tension: 0.4,
            borderWidth: 3
          }
        ]
      },

      options: {

        responsive: true,

        maintainAspectRatio:
          false,

        scales: {

          y: {
            min: 0,
            max: 100
          }
        }
      }
    }
  );

// ================= UPDATE CHART =================

function updateChart() {

  waterChart.data
    .labels.push("");

  waterChart.data
    .datasets[0]
    .data.push(
      tankLevel
    );

  if (
    waterChart.data
      .datasets[0]
      .data.length >
    25
  ) {

    waterChart.data
      .labels.shift();

    waterChart.data
      .datasets[0]
      .data.shift();
  }

  waterChart.update();
}

// ================= UPDATE TANK =================

function updateTank() {

  document
    .getElementById(
      "waterValue"
    )
    .innerHTML =
    tankLevel + "%";

  document
    .getElementById(
      "tankPercent"
    )
    .innerHTML =
    tankLevel + "%";

  document
    .getElementById(
      "tankText"
    )
    .innerHTML =
    tankLevel + "%";

  document
    .getElementById(
      "waterFill"
    )
    .style.height =
    tankLevel + "%";

  updateChart();
}

// ================= MQTT CONNECT =================

client.on(
  "connect",
  () => {

    terminal(
      "MQTT Connected"
    );

    client.subscribe(
      "home/discovery"
    );

    terminal(
      "Waiting for ESP32..."
    );
  }
);
// ================= MQTT MESSAGES =================

client.on(
  "message",
  (
    topic,
    message
  ) => {

    const msg =
      message.toString();

    // ================= DISCOVERY =================

    if (
      topic ===
      "home/discovery"
    ) {

      if (
        deviceID === null
      ) {

        deviceID =
          msg.trim();

        document
          .getElementById(
            "deviceID"
          )
          .innerHTML =
          deviceID;

        statusTopic =
          `home/${deviceID}/device/status`;

        ackTopic =
          `home/${deviceID}/ack`;

        lightTopic =
          `home/${deviceID}/actions/light`;

        fanTopic =
          `home/${deviceID}/actions/fan`;

        pumpTopic =
          `home/${deviceID}/actions/pump`;

        wifiSSIDTopic =
          `home/${deviceID}/actions/wifi/ssid`;

        wifiPasswordTopic =
          `home/${deviceID}/actions/wifi/password`;

        wifiSaveTopic =
          `home/${deviceID}/actions/wifi/save`;

        client.subscribe(
          statusTopic
        );

        client.subscribe(
          ackTopic
        );

        terminal(
          `ESP32 Found : ${deviceID}`
        );

        addLog(
          "ESP32",
          "DISCOVERED",
          deviceID
        );
      }
    }

    // ================= ONLINE/OFFLINE =================

    if (
      topic ===
      statusTopic
    ) {

      document
        .getElementById(
          "espState"
        )
        .innerHTML =
        msg;

      deviceOnline =
        (
          msg ===
          "ONLINE"
        );

      terminal(
        `ESP32 ${msg}`
      );

      addLog(
        "ESP32",
        "STATUS",
        msg
      );
    }

    // ================= ACK =================

    if (
      topic ===
      ackTopic
    ) {

      terminal(
        msg
      );

      addLog(
        "ESP32",
        "ACK",
        msg
      );

      // FAN

      if (
        msg.includes(
          "FAN ON"
        )
      ) {

        document
          .getElementById(
            "fanState"
          )
          .innerHTML =
          "ON";
      }

      if (
        msg.includes(
          "FAN OFF"
        )
      ) {

        document
          .getElementById(
            "fanState"
          )
          .innerHTML =
          "OFF";
      }

      // LIGHT

      if (
        msg.includes(
          "LIGHT ON"
        )
      ) {

        document
          .getElementById(
            "lightState"
          )
          .innerHTML =
          "ON";
      }

      if (
        msg.includes(
          "LIGHT OFF"
        )
      ) {

        document
          .getElementById(
            "lightState"
          )
          .innerHTML =
          "OFF";
      }

      // PUMP

      if (
        msg.includes(
          "PUMP ON"
        )
      ) {

        pumpRunning =
          true;

        document
          .getElementById(
            "pumpState"
          )
          .innerHTML =
          "ON";
      }

      if (
        msg.includes(
          "PUMP OFF"
        )
      ) {

        pumpRunning =
          false;

        document
          .getElementById(
            "pumpState"
          )
          .innerHTML =
          "OFF";
      }
    }
  }
);

// ================= FAN CONTROL =================

function fanControl(
  state
) {

  if (
    !deviceOnline
  ) {

    alert(
      "ESP32 Offline"
    );

    return;
  }

  client.publish(
    fanTopic,
    state
  );

  terminal(
    `Fan ${state}`
  );

  addLog(
    "Fan",
    state,
    "Sent"
  );
}

// ================= LIGHT CONTROL =================

function lightControl(
  state
) {

  if (
    !deviceOnline
  ) {

    alert(
      "ESP32 Offline"
    );

    return;
  }

  client.publish(
    lightTopic,
    state
  );

  terminal(
    `Light ${state}`
  );

  addLog(
    "Light",
    state,
    "Sent"
  );
}

// ================= PUMP CONTROL =================

function pumpControl(
  state
) {

  if (
    !deviceOnline
  ) {

    alert(
      "ESP32 Offline"
    );

    return;
  }

  // Tank Full Protection

  if (
    state ===
      "ON" &&
    tankLevel >= 100
  ) {

    terminal(
      "Tank Full - Pump OFF"
    );

    addLog(
      "Pump",
      "ON",
      "Blocked"
    );

    alert(
      "Tank is Full"
    );

    return;
  }

  client.publish(
    pumpTopic,
    state
  );

  terminal(
    `Pump ${state}`
  );

  addLog(
    "Pump",
    state,
    "Sent"
  );

  pumpRunning =
    (
      state ===
      "ON"
    );

  document
    .getElementById(
      "pumpState"
    )
    .innerHTML =
    state;
}

// ================= EMPTY TANK =================

function emptyTank() {

  tankLevel = 0;

  pumpRunning =
    false;

  if (
    deviceOnline
  ) {

    client.publish(
      pumpTopic,
      "OFF"
    );
  }

  updateTank();

  terminal(
    "Tank Emptied"
  );

  addLog(
    "Tank",
    "EMPTY",
    "0%"
  );
}

// ================= FILL TANK =================

function fillTank() {

  tankLevel = 100;

  pumpRunning =
    false;

  if (
    deviceOnline
  ) {

    client.publish(
      pumpTopic,
      "OFF"
    );
  }

  updateTank();

  terminal(
    "Tank Filled to 100%"
  );

  addLog(
    "Tank",
    "FILL",
    "100%"
  );
}

// ================= TANK SIMULATION =================

setInterval(
  () => {

    if (
      pumpRunning &&
      tankLevel < 100
    ) {

      tankLevel += 5;

      if (
        tankLevel >
        100
      ) {

        tankLevel = 100;
      }

      updateTank();

      terminal(
        `Tank Level : ${tankLevel}%`
      );

      addLog(
        "Tank",
        "UPDATE",
        `${tankLevel}%`
      );

      // Tank Full

      if (
        tankLevel >=
        100
      ) {

        pumpRunning =
          false;

        if (
          deviceOnline
        ) {

          client.publish(
            pumpTopic,
            "OFF"
          );
        }

        document
          .getElementById(
            "pumpState"
          )
          .innerHTML =
          "OFF";

        terminal(
          "TANK FULL"
        );

        terminal(
          "PUMP AUTO OFF"
        );

        addLog(
          "Pump",
          "AUTO OFF",
          "Tank Full"
        );
      }
    }

  },
  1000
);
// ================= SIDEBAR NAVIGATION =================

// Dashboard
const dashboardBtn =
  document.getElementById(
    "dashboardBtn"
  );

if (dashboardBtn) {
  dashboardBtn.onclick = () => {

    window.scrollTo({
      top: 0,
      behavior: "smooth"
    });

    terminal(
      "Dashboard Opened"
    );
  };
}

// Devices
const devicesBtn =
  document.getElementById(
    "devicesBtn"
  );

if (devicesBtn) {
  devicesBtn.onclick = () => {

    document
      .getElementById(
        "devicesSection"
      )
      .scrollIntoView({
        behavior: "smooth"
      });

    terminal(
      "Devices Opened"
    );
  };
}

// Logs
const logsBtn =
  document.getElementById(
    "logsBtn"
  );

if (logsBtn) {
  logsBtn.onclick = () => {

    document
      .getElementById(
        "logsSection"
      )
      .scrollIntoView({
        behavior: "smooth"
      });

    terminal(
      "Logs Opened"
    );
  };
}

// ================= SETTINGS =================

const wifiModal =
  document.getElementById(
    "wifiModal"
  );

const settingsBtn =
  document.getElementById(
    "settingsBtn"
  );

if (
  settingsBtn &&
  wifiModal
) {

  settingsBtn.onclick = () => {

    wifiModal.style.display =
      "flex";

    terminal(
      "WiFi Settings Opened"
    );
  };
}

// Cancel Button
const closeWifiBtn =
  document.getElementById(
    "closeWifiBtn"
  );

if (
  closeWifiBtn &&
  wifiModal
) {

  closeWifiBtn.onclick =
    () => {

      wifiModal.style.display =
        "none";
    };
}

// ================= SAVE WIFI =================

const saveWifiBtn =
  document.getElementById(
    "saveWifiBtn"
  );

if (saveWifiBtn) {

  saveWifiBtn.onclick =
    () => {

      const ssid =
        document
          .getElementById(
            "ssidInput"
          )
          .value
          .trim();

      const password =
        document
          .getElementById(
            "passwordInput"
          )
          .value
          .trim();

      if (
        ssid === "" ||
        password === ""
      ) {

        alert(
          "Enter SSID and Password"
        );

        return;
      }

      if (
        !deviceOnline
      ) {

        alert(
          "ESP32 Offline"
        );

        return;
      }

      client.publish(
        wifiSSIDTopic,
        ssid
      );

      client.publish(
        wifiPasswordTopic,
        password
      );

      client.publish(
        wifiSaveTopic,
        "SAVE"
      );

      terminal(
        "New WiFi Sent"
      );

      terminal(
        `SSID : ${ssid}`
      );

      terminal(
        "ESP32 Restarting..."
      );

      addLog(
        "WiFi",
        "CHANGE",
        ssid
      );

      wifiModal.style.display =
        "none";
    };
}

// ================= DOWNLOAD CSV =================

const downloadBtn =
  document.getElementById(
    "downloadBtn"
  );

if (downloadBtn) {

  downloadBtn.onclick =
    () => {

      let csv =
        "Time,Device,Action,Status\n";

      logs.forEach(
        item => {

          csv +=
            `${item.time},${item.device},${item.action},${item.status}\n`;
        }
      );

      const blob =
        new Blob(
          [csv],
          {
            type:
              "text/csv"
          }
        );

      const a =
        document.createElement(
          "a"
        );

      a.href =
        URL.createObjectURL(
          blob
        );

      a.download =
        "ActivityLogs.csv";

      a.click();

      terminal(
        "CSV Downloaded"
      );
    };
}

// ================= DOWNLOAD TXT =================

const downloadLogsBtn =
  document.getElementById(
    "downloadLogsBtn"
  );

if (
  downloadLogsBtn
) {

  downloadLogsBtn.onclick =
    () => {

      let text = "";

      logs.forEach(
        item => {

          text +=
            `${item.time} | ${item.device} | ${item.action} | ${item.status}\n`;
        }
      );

      const blob =
        new Blob(
          [text],
          {
            type:
              "text/plain"
          }
        );

      const a =
        document.createElement(
          "a"
        );

      a.href =
        URL.createObjectURL(
          blob
        );

      a.download =
        "ActivityLogs.txt";

      a.click();

      terminal(
        "TXT Log Downloaded"
      );
    };
}

// ================= INITIAL TERMINAL =================

terminal(
  "Smart Water Dashboard Started"
);

terminal(
  "Connecting MQTT..."
);