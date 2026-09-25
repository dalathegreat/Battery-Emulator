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
#graph{display:flex;align-items:flex-end;height:200px;border:1px solid #ccc;cursor:pointer;touch-action:pan-y}
.bar{flex:1;border:1px solid #fff}
.row{display:flex;flex-wrap:wrap;align-items:center;justify-content:flex-end;gap:6px;margin:10px 0}
#val,.lgd{font-weight:700}
.lgd{padding:2px 8px;border:1px solid transparent;border-radius:4px}
</style>
<button onclick="location.href='/'">Back to main page</button>
<button onclick="location.href='/advanced'+location.search">More Battery Info</button>
)html";

// Voltage summary, selected cell's value, graph and legend come first, the cell table below them.
// The value readout has a line of its own above the graph: a finger sliding along the bars covers
// what is below it, not what is above, and a readout whose length changes from cell to cell would
// rewrap a row it shared with the badges, bouncing the table up and down on a phone. The legend
// badges keep their right aligned row under the graph and wrap only on a screen too narrow for them.
const char panel_start[] =
    "<div class='battery-panel'><div id='volt'></div><div id='val'>Value: ...</div><div id='graph'></div>"
    "<div class='row'><span class='lgd' style='background:blue'>Idle</span>";

// d = cell millivolts, b = per cell balancing flags, A = balancing suffix, M = empty pack message.
//
// A cell is selected rather than hovered, so one code path serves mouse, pen and touch. The
// selection lights up the bar and the table cell and prints the cell number and voltage above the
// graph, and it stays until another cell is picked, because a finger has no hover to end.
//
// In the graph the whole column counts, not just the bar, so a finger does not have to hit a bar a
// few pixels wide and can slide along the pack to scrub through the cells. touch-action:pan-y keeps
// the browser from cancelling that slide as a pan, while vertical swipes still scroll the page. The
// column comes from the pointer's x position inside the graph's border, which also follows a finger
// that has slid off the bar it started on: the bars share the width equally (flex:1), and
// scrollWidth still spans all of them when 192 bars overflow a phone screen. Table cells select on
// a tap or a mouse hover, so a swipe that scrolls the table does not move the selection.
//
// H(cell, selected) holds both colour schemes, so the bars take their initial colour from it too. A
// selected bar turns white whatever its colour: the lighter cyan a balancing bar used to get was
// next to invisible on the two pixel wide bars of a phone.
const char page_script[] = R"html(<script>
const G=document.getElementById('graph'),V=document.getElementById('val'),C=document.getElementById('cells'),T=document.getElementById('volt');
if(d.length){
let p=0;
const mn=Math.min(...d),mx=Math.max(...d),lo=mn-20,sc=180/(mx-mn+40),
H=(j,o,z=b[j])=>{G.children[j].style.background=o?'#fff':z?'#0ff':'blue';C.children[j].style.background=o?z?'#066':'blue':''},
S=i=>{H(p,0);H(p=i,1);V.textContent='Cell '+(i+1)+': '+d[i]+' mV'+(b[i]?' (balancing)':'')};
T.innerHTML='Min/Max: '+mn+'/'+mx+' mV<br>Delta: '+(mx-mn)+' mV'+A;
d.forEach((mV,i)=>{
const e=document.createElement('div'),r=document.createElement('div');
e.className='cell';
e.innerHTML='<span'+(mV<3000?' class=lv>':'>')+'Cell '+(i+1)+'<br>'+mV+' mV</span>';
e.onclick=e.onmouseover=()=>S(i);
r.className='bar';
r.style.height=(mV-lo)*sc+20+'px';
if(b[i])r.style.borderColor='#0ff';
if(mV==mn||mV==mx){e.style.borderColor='red';r.style.borderColor='red'}
G.appendChild(r);C.appendChild(e);H(i,0)});
G.onpointerdown=G.onpointermove=v=>{const i=(v.clientX-G.getBoundingClientRect().left-G.clientLeft)/G.scrollWidth*d.length|0;i in d&&S(i)}}
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
  // Outlined rather than filled, the way a min/max bar is marked in the graph.
  content += "<span class='lgd' style='border-color:red'>Min/Max</span></div>";
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
    content += " (balancing now!)";
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
