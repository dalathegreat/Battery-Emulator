#include "cellmonitor_html.h"
#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"

// =========================================================================
// 🎨 1. CSS & HTML HEADER (Hybrid Responsive Edition)
// =========================================================================
const char CELLMONITOR_HTML_START[] PROGMEM = R"rawliteral(
<style>
  .cm-wrap { display: flex; flex-direction: column; gap: 20px; }
  .bat-card { background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); border-top: 4px solid #3498db; padding: 20px; }
  .bat-header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #f0f0f0; padding-bottom: 15px; margin-bottom: 15px; flex-wrap: wrap; gap: 10px; }
  .bat-title { margin: 0; color: #2c3e50; font-size: 1.3rem; font-weight: 800; }
  
  /* --- 📊 Stats Grid --- */
  .stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 10px; margin-bottom: 20px; }
  .stat-box { background: #f8f9fa; border: 1px solid #e9ecef; border-left: 4px solid #3498db; padding: 12px; border-radius: 6px; }
  .stat-box.max { border-left-color: #e74c3c; }
  .stat-box.min { border-left-color: #f39c12; }
  .stat-box.delta { border-left-color: #9b59b6; }
  .stat-box h4 { margin: 0; font-size: 0.8rem; color: #7f8c8d; text-transform: uppercase; }
  .stat-box .val { font-size: 1.4rem; font-weight: bold; color: #333; margin-top: 5px; }

  /* --- 📈 Graph Wrapper --- */
  .graph-wrap { position: relative; background: #fff; border: 1px solid #e0e0e0; border-radius: 6px; padding: 10px; margin-top: 10px; box-shadow: inset 0 2px 5px rgba(0,0,0,0.02); min-height: 200px;}
  
  /* --- 📱 Mobile Bars CSS --- */
  .m-bars { display: flex; flex-direction: column; gap: 6px; }
  .m-cell { position: relative; background: #f8f9fa; border: 1px solid #eee; border-radius: 4px; height: 38px; display: flex; align-items: center; overflow: hidden; box-shadow: 0 1px 2px rgba(0,0,0,0.02); }
  .m-bar { position: absolute; left: 0; top: 0; height: 100%; z-index: 1; transition: width 0.3s; opacity: 0.75; }
  .m-text { position: relative; z-index: 2; display: flex; justify-content: space-between; width: 100%; padding: 0 12px; font-size: 0.95rem; font-weight: 700; font-family: 'Consolas', monospace; }
  .m-num { color: #7f8c8d; }
  .m-cell.max { border-color: #fadbd8; background: #fdedec; }
  .m-cell.min { border-color: #fdebd0; background: #fef5e7; }
  
  /* --- 🚥 Legend --- */
  .legend-box { display: flex; gap: 15px; flex-wrap: wrap; font-size: 0.85rem; color: #666; justify-content: flex-end; margin-top: -10px; }
  .l-item { display: flex; align-items: center; gap: 5px; }
  .l-color { width: 12px; height: 12px; border-radius: 50%; }
</style>
<div class="cm-wrap">
)rawliteral";

// =========================================================================
// 🧠 2. JAVASCRIPT LOGIC (Hybrid Renderer: Canvas + Mobile List)
// =========================================================================
const char CELLMONITOR_HTML_JS_START[] PROGMEM = R"rawliteral(
</div>
<script>
  function renderHybridChart(id, data, balancing) {
    const wrap = document.getElementById('wrap_' + id);
    if(!data || data.length === 0 || !wrap) return;

    const validData = data.filter(v => v > 0);
    if(validData.length === 0) return;

    const min_mv = Math.min(...validData);
    const max_mv = Math.max(...validData);
    const deviation = max_mv - min_mv;
    const min_index = data.indexOf(min_mv);
    const max_index = data.indexOf(max_mv);

    let currentMode = '';

    // Mobile graph drawing function (vertical).
    function drawMobile() {
        currentMode = 'mobile';
        let html = '<div class="m-bars">';
        
        // Calculate the color bar scale to zoom in and clearly see the differences.
        const yPadding = Math.max(15, deviation * 0.5);
        const yMin = min_mv - yPadding;
        const yMax = max_mv + yPadding;
        const yRange = yMax - yMin;

        data.forEach((mV, index) => {
            if(mV === 0) return;
            let pct = ((mV - yMin) / yRange) * 100;
            if(pct < 3) pct = 3; if(pct > 100) pct = 100;

            let isBal = balancing[index];
            let isMax = index === max_index;
            let isMin = index === min_index;

            // Green = Normal, Blue = Balance, Red = Max, Orange = Min
            let barColor = isBal ? '#00bcd4' : (isMax ? '#e74c3c' : (isMin ? '#f39c12' : '#3498db'));
            let bgClass = isMax ? 'max' : (isMin ? 'min' : '');

            html += `
            <div class="m-cell ${bgClass}">
                <div class="m-bar" style="width: ${pct}%; background-color: ${barColor};"></div>
                <div class="m-text">
                    <span class="m-num">Cell ${index + 1}</span>
                    <span style="color: ${isMax?'#c0392b':(isMin?'#d68910':'#2c3e50')}">${mV} mV ${isBal?'⚡':''}</span>
                </div>
            </div>`;
        });
        html += '</div>';
        wrap.innerHTML = html;
    }

    // Computer graphing function (horizontal).
    function drawDesktop() {
        currentMode = 'desktop';
        wrap.innerHTML = `<canvas id="cvs_${id}" style="width:100%; height:300px; cursor:crosshair;"></canvas>
                          <div id="tt_${id}" style="position:absolute; display:none; background:rgba(20,30,40,0.95); color:#fff; padding:12px; border-radius:6px; font-size:13px; pointer-events:none; z-index:10; transform:translate(-50%, -120%); box-shadow:0 4px 10px rgba(0,0,0,0.3); white-space:nowrap; border:1px solid #444;"></div>`;

        const canvas = document.getElementById('cvs_' + id);
        const ctx = canvas.getContext('2d');
        const tt = document.getElementById('tt_' + id);

        const yPadding = Math.max(15, deviation * 0.5);
        const yMin = min_mv - yPadding;
        const yMax = max_mv + yPadding;
        const yRange = yMax - yMin;

        function drawCvs(hoverIndex = -1) {
            const rect = wrap.getBoundingClientRect();
            const dpr = window.devicePixelRatio || 1;
            canvas.width = rect.width * dpr;
            canvas.height = 300 * dpr;
            ctx.scale(dpr, dpr);

            const w = rect.width;
            const h = 300;
            const pad = { t: 40, r: 20, b: 40, l: 55 }; 

            const getX = (i) => pad.l + (i * (w - pad.l - pad.r) / Math.max(1, data.length - 1));
            const getY = (v) => h - pad.b - ((v - yMin) / yRange) * (h - pad.t - pad.b);

            ctx.clearRect(0, 0, w, h);

            // Deviation Background
            const devYTop = getY(max_mv);
            const devYBot = getY(min_mv);
            ctx.fillStyle = 'rgba(52, 152, 219, 0.08)';
            ctx.fillRect(pad.l, devYTop, w - pad.l - pad.r, devYBot - devYTop);
            
            // Grid Lines
            ctx.fillStyle = '#7f8c8d';
            ctx.font = '11px Arial';
            ctx.textAlign = 'right';
            ctx.textBaseline = 'middle';
            for(let i=0; i<=5; i++) {
                let v = yMin + (yRange * i / 5);
                let y = getY(v);
                ctx.fillText(Math.round(v) + " mV", pad.l - 10, y);
                ctx.beginPath(); ctx.moveTo(pad.l, y); ctx.lineTo(w - pad.r, y); 
                ctx.strokeStyle = 'rgba(0,0,0,0.05)';
                ctx.stroke();
            }

            ctx.textAlign = 'center';
            ctx.textBaseline = 'top';
            let step = Math.ceil(data.length / 20); 
            for(let i=0; i<data.length; i+=step) {
                ctx.fillText(i+1, getX(i), h - pad.b + 10);
            }

            // Bars
            const barWidth = Math.max(2, ((w - pad.l - pad.r) / data.length) * 0.6);
            data.forEach((v, i) => {
                if(v === 0) return;
                const x = getX(i);
                const y = getY(v);
                const base = getY(yMin);
                ctx.fillStyle = balancing[i] ? '#00bcd4' : '#2ecc71';
                if(hoverIndex === i) ctx.fillStyle = '#f1c40f';
                ctx.fillRect(x - barWidth/2, y, barWidth, base - y);
            });

            // Line
            ctx.beginPath();
            let first = true;
            data.forEach((v, i) => {
                if(v === 0) return;
                const x = getX(i);
                const y = getY(v);
                if(first) { ctx.moveTo(x, y); first = false; }
                else ctx.lineTo(x, y);
            });
            ctx.strokeStyle = '#2c3e50';
            ctx.lineWidth = 2.5;
            ctx.stroke();

            // Dots
            data.forEach((v, i) => {
                if(v === 0) return;
                ctx.beginPath();
                ctx.arc(getX(i), getY(v), hoverIndex === i ? 5 : 2.5, 0, Math.PI*2);
                ctx.fillStyle = hoverIndex === i ? '#e74c3c' : '#fff';
                ctx.fill();
                ctx.strokeStyle = '#2c3e50';
                ctx.lineWidth = 1.5;
                ctx.stroke();
            });
        }

        drawCvs();

        // Mouse Hover Event
        canvas.addEventListener('mousemove', (e) => {
            const rect = canvas.getBoundingClientRect();
            const mouseX = e.clientX - rect.left;
            const padL = 55;
            const segment = (rect.width - padL - 20) / Math.max(1, data.length - 1);
            
            let index = Math.round((mouseX - padL) / segment);
            if (index < 0) index = 0;
            if (index >= data.length) index = data.length - 1;
            if (data[index] === 0) return;

            const px = padL + (index * segment);
            const py = 300 - 40 - ((data[index] - yMin) / yRange) * (300 - 40 - 40);

            drawCvs(index);

            let devFromMin = data[index] - min_mv;
            tt.style.display = 'block';
            tt.style.left = px + 'px';
            tt.style.top = py + 'px';
            tt.innerHTML = `
                <div style="color:#aaa; font-weight:bold; margin-bottom:5px;">Cell #${index+1}</div>
                <div style="font-size:1.4rem; font-weight:bold; color:#2ecc71;">${data[index]} <span style="font-size:0.9rem;">mV</span></div>
                <div style="color:#ccc; margin-top:5px; font-size:0.85rem;">Dev from Min: <span style="color:#f39c12;">+${devFromMin} mV</span></div>
            `;
        });
        canvas.addEventListener('mouseleave', () => { tt.style.display = 'none'; drawCvs(-1); });
    }

    // Automatically switch graphs according to screen size.
    function render() {
        let newMode = window.innerWidth <= 768 ? 'mobile' : 'desktop';
        if (currentMode !== newMode) {
            if (newMode === 'mobile') drawMobile();
            else drawDesktop();
        } else if (newMode === 'desktop') {
            drawDesktop(); // Redraw canvas on resize
        }
    }

    render();
    window.addEventListener('resize', render);
  }
)rawliteral";

// =========================================================================
// 🔚 3. HTML FOOTER (Auto Reload)
// =========================================================================
const char CELLMONITOR_HTML_END[] PROGMEM = R"rawliteral(
  setTimeout(function(){ location.reload(true); }, 20000);
</script>
)rawliteral";

// =========================================================================
// 🛠️ 4. HELPER FUNCTIONS
// =========================================================================
String generateBatteryBlock(DATALAYER_BATTERY_TYPE& batLayer, const String& title, const String& id) {
    if (batLayer.info.number_of_cells == 0) return ""; 

    uint16_t max_mv = batLayer.status.cell_max_voltage_mV;
    uint16_t min_mv = batLayer.status.cell_min_voltage_mV;
    uint16_t delta_mv = max_mv - min_mv;

    String html = "<div class='bat-card'>";
    
    // Header
    html += "<div class='bat-header'>";
    html += "<h3 class='bat-title'>🔋 " + title + "</h3>";
    html += "<div class='legend-box'>";
    html += "<div class='l-item'><div class='l-color' style='background:#2ecc71;'></div> Normal</div>";
    html += "<div class='l-item'><div class='l-color' style='background:#e74c3c;'></div> Max</div>";
    html += "<div class='l-item'><div class='l-color' style='background:#f39c12;'></div> Min</div>";
    html += "<div class='l-item'><div class='l-color' style='background:#00bcd4;'></div> Balancing</div>";
    html += "</div></div>"; 

    // Stats Grid
    html += "<div class='stats-grid'>";
    html += "<div class='stat-box max'><h4>Highest</h4><div class='val'>" + String(max_mv) + " <span style='font-size:0.8rem;'>mV</span></div></div>";
    html += "<div class='stat-box min'><h4>Lowest</h4><div class='val'>" + String(min_mv) + " <span style='font-size:0.8rem;'>mV</span></div></div>";
    html += "<div class='stat-box delta'><h4>Delta</h4><div class='val'>" + String(delta_mv) + " <span style='font-size:0.8rem;'>mV</span></div></div>";
    html += "<div class='stat-box'><h4>Total Cells</h4><div class='val'>" + String(batLayer.info.number_of_cells) + "</div></div>";
    html += "</div>";

    // JavaScript allows switching between graph drawing (Canvas or Mobile Bars).
    html += "<div id='wrap_" + id + "' class='graph-wrap'></div>";

    html += "</div>"; 
    return html;
}

String generateBatteryJS(DATALAYER_BATTERY_TYPE& batLayer, const String& id) {
    if (batLayer.info.number_of_cells == 0) return "";

    String js = "renderHybridChart('" + id + "', [";
    for (int i = 0; i < batLayer.info.number_of_cells; i++) {
        js += String(batLayer.status.cell_voltages_mV[i]) + ",";
    }
    js += "], [";
    for (int i = 0; i < batLayer.info.number_of_cells; i++) {
        js += batLayer.status.cell_balancing_status[i] ? "true," : "false,";
    }
    js += "]);\n";
    return js;
}

// =========================================================================
// 🚀 5. MAIN PROCESSOR
// =========================================================================
String cellmonitor_processor(const String& var) {
  if (var == "X") {
    String html = FPSTR(CELLMONITOR_HTML_START);
    
    html += generateBatteryBlock(datalayer.battery, "Main Battery Pack", "b1");
    if (battery2) html += generateBatteryBlock(datalayer.battery2, "Battery Pack #2", "b2");
    if (battery3) html += generateBatteryBlock(datalayer.battery3, "Battery Pack #3", "b3");
    
    html += FPSTR(CELLMONITOR_HTML_JS_START);

    html += generateBatteryJS(datalayer.battery, "b1");
    if (battery2) html += generateBatteryJS(datalayer.battery2, "b2");
    if (battery3) html += generateBatteryJS(datalayer.battery3, "b3");
    
    html += FPSTR(CELLMONITOR_HTML_END);
    return html;
  }
  return String();
}