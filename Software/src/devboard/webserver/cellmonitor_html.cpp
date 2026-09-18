#include "cellmonitor_html.h"
#include <Arduino.h>
#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "index_html.h"

namespace {

// One pack is rendered per request, selected by the tab strip, the way the More Battery Info page
// works. The markup and the script below are therefore written once instead of once per battery.
const DATALAYER_BATTERY_TYPE& battery_data(unsigned index) {
  if (index == 1) {
    return datalayer.battery2;
  }
  if (index == 2) {
    return datalayer.battery3;
  }
  return datalayer.battery;
}

// Battery 1 always has a panel, so the page still explains itself before a battery type is picked.
bool battery_present(unsigned index) {
  if (index == 1) {
    return battery2 != nullptr;
  }
  if (index == 2) {
    return battery3 != nullptr;
  }
  return true;
}

// Legend label for the cyan bars. A BMS can flag cells for balancing long before it actually bleeds
// them, so when the aggregate status reports that waiting state, label the bars as pending instead.
const char* balancing_legend_label(balancing_status_enum status) {
  return (status == BALANCING_STATUS_BLOCKED) ? "Pending" : "Balancing";
}

// Appending through a stack buffer keeps the per-cell String temporaries (and their allocations)
// out of the loop that runs once for every cell in the pack.
void append_uint(String& out, unsigned value, const char* suffix) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%u%s", value, suffix);
  out += buffer;
}

// Page chrome. The More Battery Info link carries location.search along, so the pack selected here
// stays selected there; the browser assembles it, nothing is built on the ESP.
const char page_head[] =
    INDEX_HTML_SUBPAGE_STYLE R"html(.container{display:flex;flex-wrap:wrap;justify-content:space-around}
.cell{padding:10px;border:1px solid #fff;text-align:center}
.lv{color:red}
#graph{display:flex;align-items:flex-end;height:200px;border:1px solid #ccc;position:relative}
.bar{display:inline-block;position:relative;cursor:pointer;border:1px solid #fff}
.row{display:flex;flex-wrap:wrap;align-items:center;justify-content:flex-end;gap:6px;margin:10px 0}
#val,.lgd{font-weight:700}
#val{margin-right:auto}
.lgd{padding:2px 8px;border-radius:4px}
</style>
<button onclick="location.href='/'">Back to main page</button>
<button onclick="location.href='/advanced'+location.search">More Battery Info</button>
)html";

// Graph, hovered value and legend come first, the cell table below them. The value readout and the
// legend badges share one flex row: the auto margin holds the readout left and pushes the badges
// right, and once the row runs out of width they wrap onto lines of their own, still right aligned.
const char panel_start[] =
    "<div class='battery-panel'><div id='volt'></div><div id='graph'></div><div class='row'>"
    "<span id='val'>Value: ...</span><span class='lgd' style='background:blue'>Idle</span>";

// d = cell millivolts, b = per cell balancing flags, A = balancing suffix, M = empty pack message.
const char page_script[] = R"html(<script>
const G=document.getElementById('graph'),V=document.getElementById('val'),C=document.getElementById('cells'),T=document.getElementById('volt');
if(d.length){
const mn=Math.min(...d),mx=Math.max(...d),lo=mn-20,sc=180/(mx-mn+40),w=750/d.length+'px';
T.innerHTML='Max Voltage: '+mx+' mV<br>Min Voltage: '+mn+' mV<br>Voltage Deviation: '+(mx-mn)+' mV'+A;
d.forEach((mV,i)=>{
const e=document.createElement('div'),r=document.createElement('div'),z=b[i];
e.className='cell';
e.innerHTML='<span'+(mV<3000?' class=lv>':'>')+'Cell '+(i+1)+'<br>'+mV+' mV</span>';
r.className='bar';
r.style.height=(mV-lo)*sc+20+'px';
r.style.width=w;
r.style.background=z?'#0ff':'blue';
if(z)r.style.borderColor='#0ff';
if(mV==mn||mV==mx){e.style.borderColor='red';r.style.borderColor='red'}
const on=()=>{V.textContent='Value: '+mV+(z?' (balancing)':'');r.style.background=z?'#8ff':'lightblue';e.style.background=z?'#066':'blue'};
const off=()=>{V.textContent='Value: ...';r.style.background=z?'#0ff':'blue';e.style.removeProperty('background')};
r.onmouseenter=e.onmouseenter=on;
r.onmouseleave=e.onmouseleave=off;
G.appendChild(r);C.appendChild(e)})}
else T.textContent=M;
setTimeout(()=>location.reload(),20000)
</script>)html";

String cellmonitor_processor(const String& var, unsigned selected) {
  if (var != "X") {
    return String();
  }

  const DATALAYER_BATTERY_TYPE& pack = battery_data(selected);
  const uint8_t cells = pack.info.number_of_cells;

  String content;
  content.reserve(4096);
  content += page_head;

  // Tab strip, shown only once a second pack exists.
  if (battery2 || battery3) {
    content += "<nav aria-label='Battery selection'>";
    for (unsigned i = 0; i < 3; i++) {
      if (!battery_present(i)) {
        continue;
      }
      char tab[112];
      snprintf(tab, sizeof(tab), "<a class='battery-tab' href='/cellmonitor?battery=%u'%s>Battery %u</a>", i + 1,
               i == selected ? " aria-current='page'" : "", i + 1);
      content += tab;
    }
    content += "</nav>";
  }

  content += panel_start;

  bool cell_balancing = false;
  for (uint8_t i = 0u; i < cells; i++) {
    if (pack.status.cell_balancing_status[i]) {
      cell_balancing = true;
      break;
    }
  }
  if (cell_balancing) {
    content += "<span class='lgd' style='background:#0ff;color:#000'>";
    content += balancing_legend_label(pack.status.balancing_status);
    content += "</span>";
  } else if (pack.status.balancing_status == BALANCING_STATUS_ACTIVE) {
    // Batteries that report no per-cell flags still say whether the pack as a whole is balancing.
    content += "<span class='lgd' style='background:#f90;color:#000'>Balancing is active now!</span>";
  }
  content += "<span class='lgd' style='background:red'>Min/Max</span></div>";
  content += "<div id='cells' class='container'></div></div>";

  content += "<script>const d=[";
  for (uint8_t i = 0u; i < cells; i++) {
    if (pack.status.cell_voltages_mV[i] == 0) {
      continue;
    }
    append_uint(content, pack.status.cell_voltages_mV[i], ",");
  }
  content += "],b=[";
  for (uint8_t i = 0u; i < cells; i++) {
    if (pack.status.cell_voltages_mV[i] == 0) {
      continue;
    }
    content += pack.status.cell_balancing_status[i] ? "1," : "0,";
  }
  content += "],A='";
  if (pack.status.balancing_status == BALANCING_STATUS_ACTIVE) {
    content += " (Battery is balancing now!)";
  }
  content += "',M='";
  if (cells > 0) {
    append_uint(content, cells, " cells configured, but cellvoltages not yet read");
  } else {
    content += "Amount of cells unknown. Cellvoltages not yet read";
  }
  content += "';</script>";

  content += page_script;
  return content;
}
}  // namespace

void send_cellmonitor_page(AsyncWebServerRequest* request) {
  unsigned selected = 0;
  if (request->hasParam("battery")) {
    const String value = request->getParam("battery")->value();
    if (value.length() != 1 || value[0] < '1' || value[0] > '3') {
      request->send(400, "text/plain", "Invalid battery selection.");
      return;
    }
    selected = value[0] - '1';
  }
  if (!battery_present(selected)) {
    request->send(404, "text/plain", "This battery is not configured.");
    return;
  }
  request->send(200, "text/html", index_html,
                [selected](const String& var) { return cellmonitor_processor(var, selected); });
}
