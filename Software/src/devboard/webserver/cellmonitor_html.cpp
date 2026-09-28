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

// Appending through a stack buffer keeps the per-cell String temporaries (and their allocations)
// out of the loop that runs once for every cell in the pack.
void append_uint(String& out, unsigned value, const char* suffix) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%u%s", value, suffix);
  out += buffer;
}

/* Everything the page draws from, as JSON: d = cell millivolts, b = per cell balancing flags (cells
   not read yet are left out of both), n = cells configured, a = the pack reports balancing active,
   p = the pack reports balancing pending. A BMS can flag cells for balancing long before it
   actually bleeds them; while the aggregate status reports that waiting state, the page labels the
   flagged bars as pending instead. The same text goes into the page and answers its polls. */
void append_cell_json(String& out, const DATALAYER_BATTERY_TYPE& pack) {
  const uint8_t cells = pack.info.number_of_cells;
  const char* separator = "";
  out += "{\"d\":[";
  for (uint8_t i = 0u; i < cells; i++) {
    if (pack.status.cell_voltages_mV[i] == 0) {
      continue;
    }
    out += separator;
    append_uint(out, pack.status.cell_voltages_mV[i], "");
    separator = ",";
  }
  out += "],\"b\":[";
  separator = "";
  for (uint8_t i = 0u; i < cells; i++) {
    if (pack.status.cell_voltages_mV[i] == 0) {
      continue;
    }
    out += separator;
    out += pack.status.cell_balancing_status[i] ? "1" : "0";
    separator = ",";
  }
  out += "],\"n\":";
  append_uint(out, cells, ",\"a\":");
  out += (pack.status.balancing_status == BALANCING_STATUS_ACTIVE) ? "1" : "0";
  out += ",\"p\":";
  out += (pack.status.balancing_status == BALANCING_STATUS_BLOCKED) ? "1}" : "0}";
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
// The middle badge is filled in by the script: balancing or pending bars, or a pack that reports
// balancing without per cell flags.
const char panel[] =
    "<div class='battery-panel'><div id='volt'></div><div id='val'>Value: ...</div><div id='graph'></div>"
    "<div class='row'><span class='lgd' style='background:blue'>Idle</span>"
    "<span id='lgd' class='lgd' style='color:#000' hidden></span>"
    // Outlined rather than filled, the way a min/max bar is marked in the graph.
    "<span class='lgd' style='border-color:red'>Min/Max</span></div>"
    "<div id='cells' class='container'></div></div>";

// U = where to poll, J = the data at the time the page was sent (see append_cell_json).
//
// A cell is selected rather than hovered, so one code path serves mouse, pen and touch. The
// selection lights up the bar and the table cell and prints the cell number and voltage above the
// graph, and it stays until another cell is picked, because a finger has no hover to end. It also
// survives the refresh: every 20 s the page fetches fresh data and redraws in place, instead of
// reloading and forgetting the selection. A hidden tab does not poll; it catches up when shown
// again. A failed poll dims the panel, so old values are not taken for fresh ones, and retries.
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
const g=i=>document.getElementById(i),G=g('graph'),V=g('val'),C=g('cells'),T=g('volt'),Y=g('lgd'),P=G.parentNode;
let d=[],b=[],p=-1,t;
const H=(j,o,z=b[j])=>{G.children[j].style.background=o?'#fff':z?'#0ff':'blue';C.children[j].style.background=o?z?'#066':'blue':''},
S=i=>{if(p in d)H(p,0);H(p=i,1);V.textContent='Cell '+(i+1)+': '+d[i]+' mV'+(b[i]?' (balancing)':'')},
draw=j=>{
d=j.d;b=j.b;G.textContent=C.textContent='';
const k=b.some(x=>x);
Y.textContent=k?(j.p?'Pending':'Balancing'):j.a?'Balancing is active now!':'';
Y.style.background=k?'#0ff':'#f90';Y.hidden=!Y.textContent;
if(!d.length){p=-1;V.textContent='Value: ...';T.textContent=j.n?j.n+' cells configured, but cellvoltages not yet read':'Amount of cells unknown. Cellvoltages not yet read';return}
const mn=Math.min(...d),mx=Math.max(...d),lo=mn-20,sc=180/(mx-mn+40);
T.innerHTML='Min/Max: '+mn+'/'+mx+' mV<br>Delta: '+(mx-mn)+' mV'+(j.a?' (balancing now!)':'');
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
p in d?S(p):(p=-1,V.textContent='Value: ...')},
tick=()=>{clearTimeout(t);if(document.hidden)return;let w=5000;
fetch(U,{cache:'no-store'}).then(r=>{if(!r.ok)throw 0;return r.json()}).then(j=>{draw(j);w=20000}).catch(()=>0)
.then(()=>{P.style.opacity=w>5000?'':.5;clearTimeout(t);t=setTimeout(tick,w)})};
G.onpointerdown=G.onpointermove=v=>{const i=(v.clientX-G.getBoundingClientRect().left-G.clientLeft)/G.scrollWidth*d.length|0;i in d&&S(i)};
document.addEventListener('visibilitychange',tick);
draw(J);t=setTimeout(tick,20000)
</script>)html";

String cellmonitor_processor(const String& var, unsigned selected) {
  if (var != "X") {
    return String();
  }

  const DATALAYER_BATTERY_TYPE& pack = battery_data(selected);

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

  content += panel;
  content += "<script>const U='/cellmonitor?battery=";
  append_uint(content, selected + 1, "&data=1',J=");
  append_cell_json(content, pack);
  content += ";</script>";

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
  if (request->hasParam("data")) {
    // The page's refresh: the cell data alone, a fraction of the page it used to reload.
    String json;
    json.reserve(64 + 8 * battery_data(selected).info.number_of_cells);
    append_cell_json(json, battery_data(selected));
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", json);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
    return;
  }
  request->send(200, "text/html", index_html,
                [selected](const String& var) { return cellmonitor_processor(var, selected); });
}
