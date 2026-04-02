#include "dashboard_html.h"
#include "index_html.h"

const char full_dashboard_html[] PROGMEM = INDEX_HTML_HEADER R"rawliteral(
<style>
  .battery-container { display: flex; flex-wrap: wrap; gap: 15px; justify-content: center; margin-bottom: 20px; }
  .bat-card { flex: 1; min-width: 280px; max-width: 400px; transition: 0.3s; margin-bottom: 0; box-shadow: 0 4px 15px rgba(0,0,0,0.1); border-radius: 10px; }
  .bat-card.ok { border-top: 5px solid #028d3c; background-color: #acebbd; }
  .bat-card.fault { border-top: 5px solid #e74c3c; border-top-color: #e74c3c; background-color: #fdf2f2; }
  .bat-card.hidden { display: none; }

  .details-box { margin-top: 15px; padding-top: 15px; border-top: 1px dashed #ccc; display: none; text-align: left; background: #f9f9f9; border-radius: 6px; padding: 15px; color: #333; }
  .details-box.show { display: block; animation: fadeIn 0.3s; }

  .btn-toggle { background-color: transparent; border: 1px solid #3498db; color: #3498db; display: block; width: auto; margin-top: 10px; padding: 8px; border-radius: 4px; cursor: pointer; transition: 0.2s; font-weight: bold; }
  .btn-toggle:hover { background-color: #3498db; color: #fff; }

  .status-text { font-size: 0.85rem; color: #777; margin-top: 15px; text-align: center; }

  /* ⚡ Energy Flow Animation ⚡ */
  .flow-wrapper { display: flex; align-items: center; justify-content: space-between; background: #1a1a1a; padding: 20px 15px; border-radius: 8px; margin: 15px 0; border: 1px solid #2a2a2a; box-shadow: inset 0 0 20px rgba(0,0,0,0.8); }
  .flow-box { background: #1f1f1f; padding: 12px 10px; border-radius: 6px; font-weight: bold; text-align: center; width: 85px; border: 1px solid #444; color: #eee; box-shadow: 0 0 10px rgba(0,0,0,0.8); z-index: 2; font-size: 0.9rem; letter-spacing: 1px;}
  .flow-wire { flex-grow: 1; height: 30px; background: #050505; margin: 0 -5px; position: relative; overflow: hidden; border-top: 1px solid #1a1a1a; border-bottom: 1px solid #1a1a1a; z-index: 1; }
  .flow-particles { width: 200vw; height: 30px; position: absolute; left: 0; }

  @keyframes flow-charge { from { transform: translateX(-150px); } to { transform: translateX(0px); } }
  @keyframes flow-discharge { from { transform: translateX(0px); } to { transform: translateX(-150px); } }

  .status-charging {
    animation: flow-charge 0.8s linear infinite;
    background-image:
      linear-gradient(90deg, transparent 0px, rgba(46,204,113,0) 20px, rgba(46,204,113,0.9) 60px, #fff 75px, rgba(46,204,113,0.9) 90px, rgba(46,204,113,0) 130px, transparent 150px),
      linear-gradient(90deg, transparent 0px, rgba(52,152,219,0) 20px, rgba(52,152,219,0.9) 60px, #fff 75px, rgba(52,152,219,0.9) 90px, rgba(52,152,219,0) 130px, transparent 150px),
      linear-gradient(90deg, transparent 0px, rgba(155,89,182,0) 20px, rgba(155,89,182,0.9) 60px, #fff 75px, rgba(155,89,182,0.9) 90px, rgba(155,89,182,0) 130px, transparent 150px);
    background-size: 150px 4px;
    background-position: 0px 5px, 50px 13px, 100px 21px;
    background-repeat: repeat-x;
  }
  .status-discharging {
    animation: flow-discharge 0.8s linear infinite;
    background-image:
      linear-gradient(90deg, transparent 0px, rgba(231,76,60,0) 20px, rgba(231,76,60,0.9) 60px, #fff 75px, rgba(231,76,60,0.9) 90px, rgba(231,76,60,0) 130px, transparent 150px),
      linear-gradient(90deg, transparent 0px, rgba(243,156,18,0) 20px, rgba(243,156,18,0.9) 60px, #fff 75px, rgba(243,156,18,0.9) 90px, rgba(243,156,18,0) 130px, transparent 150px),
      linear-gradient(90deg, transparent 0px, rgba(241,196,15,0) 20px, rgba(241,196,15,0.9) 60px, #fff 75px, rgba(241,196,15,0.9) 90px, rgba(241,196,15,0) 130px, transparent 150px);
    background-size: 150px 4px;
    background-position: 0px 5px, 50px 13px, 100px 21px;
    background-repeat: repeat-x;
  }
  .status-idle { opacity: 0; }

</style>

<div class="card card-warning" style="padding: 15px;">
  <div style="display: flex; justify-content: space-between; align-items: flex-start; flex-wrap: wrap; gap: 10px;">
    <h2 style="margin-top:0; color:#333;">⚡ System Overview</h2>
  <h4 style="color: #555; margin: 5px 0;">Uptime: <span id="sys_uptime" style="font-weight: normal;">--</span> <br>RAM: <span id="sys_ram" style="font-weight: normal;">--</span><br><span style="color: #9b59b6;">PSRAM: <span id="sys_psram" style="font-weight: normal;">--</span></span></h4>

  <div style="margin-top: 15px; padding: 10px; background: #f8f9fa; border: 1px solid #eee; border-radius: 4px; font-size: 0.95em; color: #555;">
    <b>Inverter:</b> <strong id="sys_inv" style="color: #2ecc71;">--</strong> &nbsp; 🔗 &nbsp;
    <b>Battery:</b> <strong id="sys_bat" style="color: #3498db;">--</strong>
  </div>
</div>

<div class="flow-wrapper">
    <div class="flow-box">INVERTER</div>
    <div class="flow-wire"><div id="energy-flow" class="flow-particles status-idle"></div></div>
    <div class="flow-box">BATTERY</div>
</div>

<div class="battery-container">
  <div id="card_b1" class="card bat-card hidden">
    <h3 style="margin-top: 0; color: #333; display: flex; justify-content: space-between; align-items: center;">
      <span>🔋 Main Battery</span>
      <span style="font-size: 0.85em; font-weight:normal; color: #555;">Status: <strong id="b1_stat" style="color:#B9B9C7;">--</strong></span>
    </h3>
    <h4 style="color: #555;">SOC: <strong id="b1_soc" style="color:#000;">--</strong>&#37; | SOH: <strong id="b1_soh" style="color:#000;">--</strong>&#37;</h4>
    <h4 style="margin: 10px 0;"><span id="b1_v" style="color: #2ecc71; font-size: 1.4em;">--</span> V &nbsp;&nbsp; <span id="b1_a" style="color: #3498db; font-size: 1.4em;">--</span> A</h4>
    <h4 style="color: #555;">Power: <span id="b1_p" style="color:#000;">--</span> W</h4>
    <h4 style="color: #555;">Cell (Min/Max): <span id="b1_cmin" style="color:#000;">--</span> / <span id="b1_cmax" style="color:#000;">--</span> mV</h4>
    <h4 style="color: #555;">Temp: <span id="b1_tmin" style="color:#000;">--</span> / <span id="b1_tmax" style="color:#000;">--</span> &deg;C</h4>

    <button class="btn-toggle" onclick="toggleDetails('b1_more')">🔽 Show / Hide Details</button>
    <div id="b1_more" class="details-box">
      <h4>⚡ Activity: <span id="b1_act" style="color:#3498db;">--</span></h4>
      <hr style="border: 0; border-top: 1px solid #ddd; margin: 10px 0;">
      <h4>Total Capacity: <span id="b1_tot" style="font-weight:normal;">--</span> Wh</h4>
      <h4>Remaining: <span id="b1_rem" style="font-weight:normal;">--</span> Wh</h4>
      <h4>Max Charge: <span id="b1_mc" style="font-weight:normal;">--</span> W</h4>
      <h4>Max Discharge: <span id="b1_md" style="font-weight:normal;">--</span> W</h4>
      <hr style="border: 0; border-top: 1px solid #ddd; margin: 10px 0;">
      <h4>🔌 Contactor Allowed:</h4>
      <h4 style="margin-left: 20px;">Battery: <span id="b1_cont_b" style="font-weight:normal;">--</span> | Inverter: <span id="sys_cont_i" style="font-weight:normal;">--</span></h4>
    </div>
  </div>

  <div id="card_b2" class="card bat-card hidden">
    <h3 style="margin-top: 0; color: #333; display: flex; justify-content: space-between; align-items: center;">
      <span>🔋 Battery 2</span>
      <span style="font-size: 0.85em; font-weight:normal; color: #555;">Status: <strong id="b2_stat" style="color:#B9B9C7;">--</strong></span>
    </h3>
    <h4 style="color: #555;">SOC: <strong id="b2_soc" style="color:#000;">--</strong>&#37; | SOH: <strong id="b2_soh" style="color:#000;">--</strong>&#37;</h4>
    <h4 style="margin: 10px 0;"><span id="b2_v" style="color: #2ecc71; font-size: 1.4em;">--</span> V &nbsp;&nbsp; <span id="b2_a" style="color: #3498db; font-size: 1.4em;">--</span> A</h4>
    <h4 style="color: #555;">Power: <span id="b2_p" style="color:#000;">--</span> W</h4>
    <h4 style="color: #555;">Cell (Min/Max): <span id="b2_cmin" style="color:#000;">--</span> / <span id="b2_cmax" style="color:#000;">--</span> mV</h4>
    <h4 style="color: #555;">Temp: <span id="b2_tmin" style="color:#000;">--</span> / <span id="b2_tmax" style="color:#000;">--</span> &deg;C</h4>

    <button class="btn-toggle" onclick="toggleDetails('b2_more')">🔽 Show / Hide Details</button>
    <div id="b2_more" class="details-box">
      <h4>⚡ Activity: <span id="b2_act" style="color:#3498db;">--</span></h4>
      <hr style="border: 0; border-top: 1px solid #ddd; margin: 10px 0;">
      <h4>Total Capacity: <span id="b2_tot" style="font-weight:normal;">--</span> Wh</h4>
      <h4>Remaining: <span id="b2_rem" style="font-weight:normal;">--</span> Wh</h4>
      <h4>Max Charge: <span id="b2_mc" style="font-weight:normal;">--</span> W</h4>
      <h4>Max Discharge: <span id="b2_md" style="font-weight:normal;">--</span> W</h4>
      <hr style="border: 0; border-top: 1px solid #ddd; margin: 10px 0;">
      <h4>🔌 Contactor Allowed:</h4>
      <h4 style="margin-left: 20px;">Battery: <span id="b2_cont_b" style="font-weight:normal;">--</span> | Inverter: <span id="sys_cont_i" style="font-weight:normal;">--</span></h4>
    </div>
  </div>

  <div id="card_b3" class="card bat-card hidden">
    <h3 style="margin-top: 0; color: #333; display: flex; justify-content: space-between; align-items: center;">
      <span>🔋 Battery 3</span>
      <span style="font-size: 0.85em; font-weight:normal; color: #555;">Status: <strong id="b3_stat" style="color:#B9B9C7;">--</strong></span>
    </h3>
    <h4 style="color: #555;">SOC: <strong id="b3_soc" style="color:#000;">--</strong>&#37; | SOH: <strong id="b3_soh" style="color:#000;">--</strong>&#37;</h4>
    <h4 style="margin: 10px 0;"><span id="b3_v" style="color: #2ecc71; font-size: 1.4em;">--</span> V &nbsp;&nbsp; <span id="b3_a" style="color: #3498db; font-size: 1.4em;">--</span> A</h4>
    <h4 style="color: #555;">Power: <span id="b3_p" style="color:#000;">--</span> W</h4>
    <h4 style="color: #555;">Cell (Min/Max): <span id="b3_cmin" style="color:#000;">--</span> / <span id="b3_cmax" style="color:#000;">--</span> mV</h4>
    <h4 style="color: #555;">Temp: <span id="b3_tmin" style="color:#000;">--</span> / <span id="b3_tmax" style="color:#000;">--</span> &deg;C</h4>

    <button class="btn-toggle" onclick="toggleDetails('b3_more')">🔽 Show / Hide Details</button>
    <div id="b3_more" class="details-box">
      <h4>⚡ Activity: <span id="b3_act" style="color:#3498db;">--</span></h4>
      <hr style="border: 0; border-top: 1px solid #ddd; margin: 10px 0;">
      <h4>Total Capacity: <span id="b3_tot" style="font-weight:normal;">--</span> Wh</h4>
      <h4>Remaining: <span id="b3_rem" style="font-weight:normal;">--</span> Wh</h4>
      <h4>Max Charge: <span id="b3_mc" style="font-weight:normal;">--</span> W</h4>
      <h4>Max Discharge: <span id="b3_md" style="font-weight:normal;">--</span> W</h4>
      <hr style="border: 0; border-top: 1px solid #ddd; margin: 10px 0;">
      <h4>🔌 Contactor Allowed:</h4>
      <h4 style="margin-left: 20px;">Battery: <span id="b3_cont_b" style="font-weight:normal;">--</span> | Inverter: <span id="sys_cont_i" style="font-weight:normal;">--</span></h4>
    </div>
  </div>
</div>

<div id="card_chg" class="card card-warning hidden" style="border-top-color: #e67e22;">
  <h3 style="margin-top:0; color:#333;">🔌 Charger Output</h3>
  <h4><span id="chg_v" style="font-size: 1.4em; color: #2ecc71;">--</span> V &nbsp;&nbsp; <span id="chg_a" style="font-size: 1.4em; color: #3498db;">--</span> A</h4>
</div>

<div class="status-text">Last Updated: <span id="last_update">Waiting for data...</span></div>

<script>
  function toggleDetails(id) { document.getElementById(id).classList.toggle('show'); }

  window.isFetchingAPI = false;

  function fetchBatteryData() {
    if (window.isFetchingAPI) return;
    window.isFetchingAPI = true;

    fetch('/api/data')
      .then(response => response.json())
      .then(data => {
        if (data && data.sys && data.sys.uptime) {
          let upMatch = data.sys.uptime.match(/(\d+)\s+days,\s+(\d+)\s+hours,\s+(\d+)\s+mins,\s+(\d+)\s+secs/);
          if (upMatch) {
              let serverSecs = parseInt(upMatch[1])*86400 + parseInt(upMatch[2])*3600 + parseInt(upMatch[3])*60 + parseInt(upMatch[4]);
              if (typeof window.localUptimeSecs === 'undefined' || Math.abs(serverSecs - window.localUptimeSecs) > 2 || serverSecs < window.localUptimeSecs) {
                  window.localUptimeSecs = serverSecs;
                  document.getElementById('sys_uptime').innerText = data.sys.uptime;
              }
          }
          let freeRamKB = Math.round(data.sys.heap / 1024);
          let totalRamKB = Math.round(data.sys.heap_size / 1024);
          let ramPercent = Math.round((data.sys.heap / data.sys.heap_size) * 100);

          document.getElementById('sys_ram').innerText = freeRamKB + " / " + totalRamKB + " KB (" + ramPercent + "%)";

          if (data.sys.psram_size > 0) {
              document.getElementById('sys_psram').innerText = Math.round(data.sys.psram / 1024) + " / " + Math.round(data.sys.psram_size / 1024) + " KB";
          } else {
              document.getElementById('sys_psram').innerText = "Not Enabled";
          }

          document.getElementById('sys_inv').innerText = data.sys.inv;
          document.getElementById('sys_bat').innerText = data.sys.bat;
          let isPaused = (data.sys.status === 'PAUSED' || data.sys.status === 'PAUSING');

          const updateBat = (id, bData) => {
            if(bData.en) {
              document.getElementById('card_' + id).classList.remove('hidden');
              document.getElementById(id + '_soc').innerText = bData.soc;
              document.getElementById(id + '_soh').innerText = bData.soh;
              document.getElementById(id + '_v').innerText = bData.v;
              document.getElementById(id + '_a').innerText = bData.a;
              document.getElementById(id + '_p').innerText = bData.p;
              document.getElementById(id + '_cmin').innerText = bData.cmin;
              document.getElementById(id + '_cmax').innerText = bData.cmax;
              document.getElementById(id + '_tmin').innerText = bData.tmin;
              document.getElementById(id + '_tmax').innerText = bData.tmax;
              document.getElementById(id + '_mc').innerText = bData.mc;
              document.getElementById(id + '_md').innerText = bData.md;
              document.getElementById(id + '_rem').innerText = bData.rem;
              document.getElementById(id + '_tot').innerText = bData.tot;
              document.getElementById('b1_cont_b').innerText = bData.b_cont ? "YES" : "NO";
              document.getElementById('sys_cont_i').innerText = data.sys.inv_cont ? "YES" : "NO";
              if(bData.fault) {
                document.getElementById('card_' + id).classList.add('fault');
                document.getElementById('card_' + id).classList.remove('ok');
              }
              else {
                document.getElementById('card_' + id).classList.add('ok');
                document.getElementById('card_' + id).classList.remove('fault');
              }

              let statElement = document.getElementById(id + '_stat');
              statElement.innerText = bData.stat;
              if (isPaused) {
                document.getElementById('card_' + id).style.opacity = '0.6';
                statElement.innerText = 'Paused ⏸️';
                statElement.style.color = '#94b1c4';
              } else {
                document.getElementById('card_' + id).style.opacity = '1.0';

                let act_str = "Idle 💤";
                if (parseFloat(bData.a) < 0) {
                  act_str = "Discharging";
                  statElement.style.color = '#f0a31f';
                }
                else if (parseFloat(bData.a) > 0) {
                  act_str = "Charging";
                  statElement.style.color = '#3441cf';
                }
                statElement.innerText = act_str;
              }
            }
          };

          let totalPowerW = 0;
          let isBatActive = false;

          if (data.b1 && data.b1.en && !data.b1.fault) { totalPowerW += parseFloat(data.b1.p) || 0; isBatActive = true; }
          if (data.b2 && data.b2.en && !data.b2.fault) { totalPowerW += parseFloat(data.b2.p) || 0; isBatActive = true; }
          if (data.b3 && data.b3.en && !data.b3.fault) { totalPowerW += parseFloat(data.b3.p) || 0; isBatActive = true; }

          let flowElement = document.getElementById('energy-flow');
          if (isBatActive && !isPaused) {
            if (totalPowerW > 50) {
              flowElement.className = 'flow-particles status-charging';
              let speed = Math.max(0.2, 2000 / totalPowerW);
              flowElement.style.animationDuration = speed + 's';
            } else if (totalPowerW < -50) {
              flowElement.className = 'flow-particles status-discharging';
              let speed = Math.max(0.2, 2000 / Math.abs(totalPowerW));
              flowElement.style.animationDuration = speed + 's';
            } else {
              flowElement.className = 'flow-particles status-idle';
              flowElement.style.animationDuration = '0s';
            }
          } else {
            flowElement.className = 'flow-particles status-idle';
            flowElement.style.animationDuration = '0s';
          }

          updateBat('b1', data.b1); updateBat('b2', data.b2); updateBat('b3', data.b3);

          if(data.chg.en) {
            document.getElementById('card_chg').classList.remove('hidden');
            document.getElementById('chg_v').innerText = data.chg.v;
            document.getElementById('chg_a').innerText = data.chg.a;
          }
          document.getElementById('last_update').innerText = new Date().toLocaleTimeString();
        }
      })
      .catch(error => console.error('Error fetching API:', error))
      .finally(() => {
          window.isFetchingAPI = false;
      });
  }

  setTimeout(() => {
    fetchBatteryData();
    setInterval(fetchBatteryData, 2000);
  }, 500);

  setInterval(function() {
    if (typeof window.localUptimeSecs !== 'undefined') {
      window.localUptimeSecs++;

      var totalSeconds = window.localUptimeSecs;
      var d = Math.floor(totalSeconds / 86400);
      totalSeconds = totalSeconds - (d * 86400);
      var h = Math.floor(totalSeconds / 3600);
      totalSeconds = totalSeconds - (h * 3600);
      var m = Math.floor(totalSeconds / 60);
      totalSeconds = totalSeconds - (m * 60);
      var s = totalSeconds;

      var timeEl = document.getElementById("sys_uptime");
      if (timeEl) {
        timeEl.innerText = d + " days, " + h + " hours, " + m + " mins, " + s + " secs";
      }
    }
  }, 1000);

</script>
)rawliteral" INDEX_HTML_FOOTER;