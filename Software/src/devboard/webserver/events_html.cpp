#include "events_html.h"
#include <limits>
#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/logging.h"
#include "../../devboard/utils/millis64.h"
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "index_html.h"

const char EVENTS_HTML_START[] = R"=====(
<style>body{background-color:#000;color:#fff}.event-log{display:flex;flex-direction:column}.event{display:flex;flex-wrap:wrap;border:1px solid #fff;padding:10px}.event>div{flex:1;min-width:90px;word-break:break-word}.event>div:first-child{flex:5;text-align:left}.event>div:nth-child(3){flex:3}.event>div:last-child{flex:1 1 100%;padding-top:4px;text-align:left}</style><div style="background-color:#303e47;padding:10px;margin-bottom:10px;border-radius:25px"><div class="event-log" id="log">
)=====";
// The table's header row carries the version of the rows below it, so the page can ask for them
// again only once something changed.
const char EVENTS_HTML_HEADER_ROW[] =
    "<div class='event' data-v='%08lx' style='background-color:#1e2c33;font-weight:700'><div>Event Type</div>"
    "<div>Severity</div><div>Last Event</div><div>Count</div><div>Data</div><div>Message</div></div>";
const char EVENTS_HTML_END[] = R"=====(
</div></div>
<style> button { background-color: #505E67; color: white; border: none; padding: 10px 20px; margin-bottom: 20px; cursor: pointer; border-radius: 10px; }
button:hover { background-color: #3A4A52; }</style>
<button onclick="askClear()">Clear all events</button>
<button onclick="home()">Back to main page</button>
<style>.event:nth-child(even){background-color:#455a64}.event:nth-child(odd){background-color:#394b52}</style>
<script>const G=document.getElementById("log"),showEvent=()=>G.querySelectorAll(".sec-ago").forEach(n=>{n.innerText=new Date(Number(BigInt(Date.now())-BigInt(n.innerText))).toLocaleString();n.className=""}),askClear=()=>confirm("Are you sure you want to clear all events?")&&(location.href="/clearevents"),home=()=>location.href="/",tick=()=>{clearTimeout(T);document.hidden||fetch("/events?since="+G.firstElementChild.dataset.v,{cache:"no-store"}).then(r=>{if(r.status!=204){if(!r.ok)throw 0;return r.text().then(h=>{G.innerHTML=h;showEvent()})}}).then(()=>G.style.opacity="",()=>G.style.opacity=.5).then(()=>{clearTimeout(T);T=setTimeout(tick,5000)})};let T;document.addEventListener("visibilitychange",tick);onload=()=>{showEvent();T=setTimeout(tick,5000)}
</script>
)=====";

static std::vector<EventData> order_events;

/* Changes whenever anything in the table changes: setting an event again bumps its count and
   timestamp (and may change its data), and clearing the events zeroes them. The page sends back
   the version it shows, and gets rows again only when this differs. */
static uint32_t events_version() {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    const EVENTS_STRUCT_TYPE* event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
    if (event_pointer->occurences == 0) {
      continue;
    }
    const uint32_t fields[] = {(uint32_t)i, event_pointer->occurences, (uint32_t)event_pointer->timestamp,
                               (uint32_t)(event_pointer->timestamp >> 32), (uint16_t)event_pointer->data};
    for (uint32_t field : fields) {
      hash = (hash ^ field) * 16777619u;
    }
  }
  return hash;
}

// The header row and one row per event that has occurred, newest first.
static void append_event_rows(String& content, uint32_t version) {
  char header[sizeof(EVENTS_HTML_HEADER_ROW) + 8];
  snprintf(header, sizeof(header), EVENTS_HTML_HEADER_ROW, (unsigned long)version);
  content.concat(header);

  const EVENTS_STRUCT_TYPE* event_pointer;

  //clear the vector
  order_events.clear();
  // Collect all events
  for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
    if (event_pointer->occurences > 0) {
      order_events.push_back({static_cast<EVENTS_ENUM_TYPE>(i), event_pointer});
    }
  }
  // Sort events by timestamp
  std::sort(order_events.begin(), order_events.end(), compareEventsByTimestampDesc);
  uint64_t current_timestamp = millis64();

  // Generate HTML and debug output
  for (const auto& event : order_events) {
    EVENTS_ENUM_TYPE event_handle = event.event_handle;
    event_pointer = event.event_pointer;

    // Get the event level string and determine background color
    String event_level = String(get_event_level_string(event_handle));
    String bg_color;
    String text_color = "#000000";

    // Set colors based on event level
    if (event_level == "INFO") {
      bg_color = "#04b34f";
    } else if (event_level == "WARNING") {
      bg_color = "#ff9900";
    } else if (event_level == "ERROR") {
      bg_color = "#a6192e";
      text_color = "#ffffff";
    } else {
      bg_color = "";
    }

    // Start event div with inline style for background color
    content.concat("<div class='event'");
    if (bg_color.length() > 0) {
      content.concat(" style='background-color: " + bg_color + "; color: " + text_color + ";'>");
    } else {
      content.concat(">");
    }

    content.concat("<div>" + String(get_event_enum_string(event_handle)) + "</div>");
    content.concat("<div>" + event_level + "</div>");
    // Frontend expects to see time difference (in ms) from now to event
    content.concat("<div class='sec-ago'>" + String(current_timestamp - event_pointer->timestamp) + "</div>");
    content.concat("<div>" + String(event_pointer->occurences) + "</div>");
    content.concat("<div>" + String(event_pointer->data) + "</div>");
    content.concat("<div>" + get_event_message_string(event_handle) + "</div>");
    content.concat("</div>");  // End of event row
  }

  //clear the vector
  order_events.clear();
}

static String events_processor(const String& var) {
  if (var == "X") {
    String content = "";
    content.reserve(5000);
    // Page format
    content.concat(FPSTR(EVENTS_HTML_START));
    append_event_rows(content, events_version());
    content.concat(FPSTR(EVENTS_HTML_END));
    return content;
  }
  return String();
}

void send_events_page(AsyncWebServerRequest* request) {
  const AsyncWebParameter* since = request->getParam("since");
  if (!since) {
    request->send(200, "text/html", index_html, events_processor);
    return;
  }
  // The page's refresh: nothing at all while the events stay as they are, the rows when they don't.
  const uint32_t version = events_version();
  char current[9];
  snprintf(current, sizeof(current), "%08lx", (unsigned long)version);
  String rows;
  if (since->value() != current) {
    rows.reserve(4000);
    append_event_rows(rows, version);
  }
  AsyncWebServerResponse* response = request->beginResponse(rows.isEmpty() ? 204 : 200, "text/html", rows);
  response->addHeader("Cache-Control", "no-store");
  request->send(response);
}

/* Script for displaying event log before it gets minified
<button onclick="askClear()">Clear all events</button>
<button onclick="home()">Back to main page</button>
<style>
    .event:nth-child(even) {
        background-color: #455a64;
    }
    .event:nth-child(odd) {
        background-color: #394b52;
    }
</style>
<script>
    var log = document.getElementById("log"), timer;
    function showEvent() {
        // Turns each "ms ago" into a local date and time, once
        log.querySelectorAll(".sec-ago").forEach(function (n) {
            n.innerText = new Date(Number(BigInt(Date.now()) - BigInt(n.innerText))).toLocaleString();
            n.className = "";
        });
    }
    function askClear() {
        if (window.confirm('Are you sure you want to clear all events?')) {
            window.location.href = '/clearevents';
        }
    }
    function home() {
        window.location.href = "/";
    }
    // Every 5 s, unless the tab is hidden: ask for the rows, quoting the version shown. The answer
    // is 204 while nothing changed, else the new rows. A failed poll dims the table and retries.
    function tick() {
        clearTimeout(timer);
        if (!document.hidden)
            fetch("/events?since=" + log.firstElementChild.dataset.v, { cache: "no-store" })
                .then(function (r) {
                    if (r.status != 204) {
                        if (!r.ok) throw 0;
                        return r.text().then(function (h) { log.innerHTML = h; showEvent(); });
                    }
                })
                .then(function () { log.style.opacity = ""; }, function () { log.style.opacity = .5; })
                .then(function () { clearTimeout(timer); timer = setTimeout(tick, 5000); });
    }
    document.addEventListener("visibilitychange", tick);
    window.onload = function () {
        showEvent();
        timer = setTimeout(tick, 5000);
    };
</script>
*/
