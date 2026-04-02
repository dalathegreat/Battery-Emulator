#include "cellmonitor_html.h"
#include <Arduino.h>
#include "index_html.h"
// =========================================================================
// Page Static + AJAX (Heatmap UI)
// =========================================================================
#define CELLMONITOR_HTML_CONTENT R"rawliteral(
<style>
  .cm-wrap { display: flex; flex-direction: column; gap: 20px; }
  .bat-card { background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); border-top: 4px solid #3498db; padding: 20px; }
  .bat-header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #f0f0f0; padding-bottom: 15px; margin-bottom: 15px; flex-wrap: wrap; gap: 10px; }
  .bat-title { margin: 0; color: #2c3e50; font-size: 1.3rem; font-weight: 800; }

  /* Stats Box */
  .stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 10px; margin-bottom: 20px; }
  .stat-box { background: #f8f9fa; border: 1px solid #e9ecef; border-left: 4px solid #3498db; padding: 12px; border-radius: 6px; }
  .stat-box.max { border-left-color: #e74c3c; }
  .stat-box.min { border-left-color: #f39c12; }
  .stat-box.delta { border-left-color: #9b59b6; }
  .stat-box h4 { margin: 0; font-size: 0.8rem; color: #7f8c8d; text-transform: uppercase; }
  .stat-box .val { font-size: 1.4rem; font-weight: bold; color: #333; margin-top: 5px; }

  /* Heatmap Grid */
  .hm-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(38px, 1fr)); gap: 6px; padding: 15px; background: #f8f9fa; border-radius: 8px; border: 1px solid #e0e0e0; box-shadow: inset 0 2px 5px rgba(0,0,0,0.02); }
  .hm-cell { display: flex; align-items: center; justify-content: center; aspect-ratio: 1; border-radius: 6px; color: white; font-weight: bold; font-size: 0.85rem; cursor: pointer; transition: transform 0.1s, filter 0.2s; user-select: none; border: 2px solid transparent; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
  .hm-cell:hover { transform: scale(1.15); z-index: 10; filter: brightness(1.1); box-shadow: 0 6px 12px rgba(0,0,0,0.3); }

  /* Balancing Animation */
  .bal-pulse { animation: pulse-border 1s infinite alternate; }
  @keyframes pulse-border { from { border-color: #00bcd4; box-shadow: 0 0 8px rgba(0, 188, 212, 0.8); } to { border-color: #ffffff; box-shadow: 0 0 2px rgba(0, 188, 212, 0.2); } }

  /* Legend & Tooltip */
  .legend-box { display: flex; gap: 15px; flex-wrap: wrap; font-size: 0.85rem; color: #666; justify-content: flex-end; margin-top: -10px; }
  .l-item { display: flex; align-items: center; gap: 5px; }
  .l-color { width: 12px; height: 12px; border-radius: 50px; }

  .hm-tooltip { position: absolute; background: rgba(20, 30, 40, 0.95); color: #fff; padding: 12px 18px; border-radius: 8px; font-size: 0.95rem; pointer-events: none; z-index: 99999; box-shadow: 0 8px 25px rgba(0,0,0,0.4); border: 1px solid #555; display: none; transform: translate(-50%, -120%); white-space: nowrap; transition: top 0.05s, left 0.05s; }
</style>

<div class="cm-wrap">
    <div id="bat_b1" style="display:none;"></div>
    <div id="bat_b2" style="display:none;"></div>
    <div id="bat_b3" style="display:none;"></div>
</div>

<div id="cm_tooltip" class="hm-tooltip"></div>

<script>
  // --- 🛠️ Tooltip Engine (Smooth Follow) ---
  let tooltipTarget = null;
  const tt = document.getElementById('cm_tooltip');

  document.addEventListener('mousemove', (e) => {
      if(tooltipTarget) {
            let rect = tooltipTarget.getBoundingClientRect();
            let centerX = rect.left + (rect.width / 2) + window.scrollX;
            let topY = rect.top + window.scrollY - rect.height;
            tt.style.left = centerX + 'px';
            tt.style.top = topY + 'px';
            let mV = tooltipTarget.getAttribute('data-v');
            let isBal = tooltipTarget.getAttribute('data-bal') === 'true';
            let cellId = tooltipTarget.innerText;

            tt.innerHTML = `
                <div style="color:#aaa; font-size:0.8rem; margin-bottom:3px;">CELL ID: #${cellId}</div>
                <div style="font-size:1.5rem; color:#2ecc71; font-weight:bold; font-family:monospace;">${mV} <span style="font-size:0.9rem;">mV</span></div>
                ${isBal ? '<div style="color:#00bcd4; font-weight:bold; margin-top:5px; font-size:0.85rem;">⚡ BALANCING ACTIVE</div>' : ''}
            `;
            tt.style.display = 'block';
      } else {
            tt.style.display = 'none';
      }
  });

  // --- 🛠️ Heatmap Renderer (Zero-Flicker DOM Updates) ---
  function renderHeatmap(id, data, balancing) {
      const wrap = document.getElementById('wrap_' + id);
      if(!wrap) return;

      const validData = data.filter(v => v > 0);
      if(validData.length === 0) return;

      const min_mv = Math.min(...validData);
      const max_mv = Math.max(...validData);
      const mean_mv = validData.reduce((a, b) => a + b, 0) / validData.length;
      const delta_mv = max_mv - min_mv;

      let grid = wrap.querySelector('.hm-grid');

      // 1. Create Grid Structure (Only on first load)
      if (!grid) {
          wrap.innerHTML = '<div class="hm-grid"></div>';
          grid = wrap.querySelector('.hm-grid');

          data.forEach((mV, index) => {
              if(mV === 0) return;
              let cell = document.createElement('div');
              cell.className = 'hm-cell';
              cell.id = `cell_${id}_${index}`;
              cell.innerText = index + 1;

              cell.onmouseenter = () => { tooltipTarget = cell; };
              cell.onmouseleave = () => { tooltipTarget = null; };

              grid.appendChild(cell);
          });
      }

      // 2. Update Cell Colors & Data (Every 2 seconds)
      data.forEach((mV, index) => {
          if(mV === 0) return;
          let cell = document.getElementById(`cell_${id}_${index}`);
          if(!cell) return;

          let isBal = balancing[index] === 1;
          let isMax = mV === max_mv;
          let isMin = mV === min_mv;

          // 🎨 Smart Color Calculation
          let bgColor = '#2ecc71'; // Normal (Green)
          if (isMax && delta_mv > 10) bgColor = '#e74c3c'; // Highest (Red) if delta > 10mV
          else if (isMin && delta_mv > 10) bgColor = '#f39c12'; // Lowest (Orange) if delta > 10mV
          else if (mV > mean_mv + 15) bgColor = '#d35400'; // Slightly High
          else if (mV < mean_mv - 15) bgColor = '#f1c40f'; // Slightly Low

          cell.style.backgroundColor = bgColor;

          if (isBal) {
              cell.classList.add('bal-pulse');
          } else {
              cell.style.borderColor = 'transparent';
              cell.classList.remove('bal-pulse');
          }

          // Update invisible data attributes for Tooltip
          cell.setAttribute('data-v', mV);
          cell.setAttribute('data-bal', isBal);
      });
  }

  // --- 🛠️ Block Generator ---
  function updateBatteryBlock(id, title, data, balancing) {
      const validData = data.filter(v => v > 0);
      if(validData.length === 0) return;

      const min_mv = Math.min(...validData);
      const max_mv = Math.max(...validData);
      const delta_mv = max_mv - min_mv;
      const cells = validData.length; // Count only connected cells

      let container = document.getElementById('bat_' + id);

      if(container.innerHTML.trim() === '') {
          container.innerHTML = `
          <div class='bat-card'>
              <div class='bat-header'>
                  <h3 class='bat-title'>🔋 ${title}</h3>
                  <div class='legend-box'>
                      <div class='l-item'><div class='l-color' style='background:#2ecc71;'></div> Normal</div>
                      <div class='l-item'><div class='l-color' style='background:#e74c3c;'></div> Max</div>
                      <div class='l-item'><div class='l-color' style='background:#f39c12;'></div> Min</div>
                      <div class='l-item'><div class='l-color' style='border:3px solid #00bcd4; background:#fff;'></div> Balancing</div>
                  </div>
              </div>
              <div class='stats-grid'>
                  <div class='stat-box max'><h4>Highest</h4><div class='val'><span id='max_${id}'>${max_mv}</span> <span style='font-size:0.8rem;'>mV</span></div></div>
                  <div class='stat-box min'><h4>Lowest</h4><div class='val'><span id='min_${id}'>${min_mv}</span> <span style='font-size:0.8rem;'>mV</span></div></div>
                  <div class='stat-box delta'><h4>Delta</h4><div class='val'><span id='del_${id}'>${delta_mv}</span> <span style='font-size:0.8rem;'>mV</span></div></div>
                  <div class='stat-box'><h4>Active Cells</h4><div class='val'>${cells}</div></div>
              </div>
              <div id='wrap_${id}' style='margin-top: 15px;'></div>
          </div>`;
          container.style.display = 'block';
      } else {
          // Update only numbers to prevent UI flicker
          document.getElementById('max_' + id).innerText = max_mv;
          document.getElementById('min_' + id).innerText = min_mv;
          document.getElementById('del_' + id).innerText = delta_mv;
      }

      renderHeatmap(id, data, balancing);
  }

  // --- 🛠️ Data Fetcher ---
  if (typeof window.isFetchingAPI === 'undefined') { window.isFetchingAPI = false; }

  function fetchCellData() {
      if (window.isFetchingAPI) return;
      window.isFetchingAPI = true;

      fetch('/api/cells')
          .then(response => response.json())
          .then(data => {
              if (data) {
                  if(data.b1 && data.b1.cv) updateBatteryBlock('b1', 'Main Battery Pack', data.b1.cv, data.b1.cb);
                  if(data.b2 && data.b2.cv) updateBatteryBlock('b2', 'Battery Pack #2', data.b2.cv, data.b2.cb);
                  if(data.b3 && data.b3.cv) updateBatteryBlock('b3', 'Battery Pack #3', data.b3.cv, data.b3.cb);
              }
          })
          .catch(error => console.error('Error fetching cell data:', error))
          .finally(() => {
              window.isFetchingAPI = false;
          });
  }

  setTimeout(() => {
      fetchCellData();
      setInterval(fetchCellData, 2000);
  }, 500);

</script>
)rawliteral"

// =========================================================================
// 🚀 PRE-COMPILED HTML (Zero RAM Allocation)
// =========================================================================
extern const char full_cellmonitor_html[] PROGMEM = INDEX_HTML_HEADER CELLMONITOR_HTML_CONTENT INDEX_HTML_FOOTER;
