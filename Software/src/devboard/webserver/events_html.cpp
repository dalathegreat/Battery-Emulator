#include "events_html.h"
#include <limits>
#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/logging.h"
#include "../../devboard/utils/millis64.h"
#include "index_html.h"

const char EVENTS_HTML_START[] PROGMEM = R"=====(
<style>
  /* ---  Base Dark Mode --- */
  body { background-color: #121212; color: #ecf0f1; font-family: 'Segoe UI', Tahoma, sans-serif; }

  /* --- Container & Header --- */
  .card-events { background: #1a252c; border-radius: 8px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); padding: 20px; border-top: 4px solid #e74c3c; margin-bottom: 20px; }
  .ev-header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #2c3e50; padding-bottom: 15px; margin-bottom: 15px; flex-wrap: wrap; gap: 10px; }
  .ev-header h3 { margin: 0; color: #ffffff; font-size: 1.25rem; }
  .btn-clear { background: #e74c3c; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; cursor: pointer; transition: 0.2s; }
  .btn-clear:hover { background: #c0392b; }
  .btn-gray { background: #505E67; } .btn-gray:hover { background: #3A4A52; }
  .btn-gray:hover { background: #3A4A52; }

  /* --- 💻 Desktop Grid Layout --- */
  .event-table { display: block; border-collapse: collapse; }
  .ev-header-row {
    display: grid;
    grid-template-columns: 2fr 1fr 1.5fr 0.8fr 1fr 3fr;
    background-color: #2c3e50;
    color: #ecf0f1;
    padding: 12px 15px;
    font-weight: bold;
    border-radius: 6px 6px 0 0;
  }
  .event {
    display: grid;
    grid-template-columns: 2fr 1fr 1.5fr 0.8fr 1fr 3fr;
    padding: 12px 15px;
    border-bottom: 1px solid #2c3e50;
    align-items: center;
    color: #bdc3c7;
  }
  .event:nth-child(even) { background-color: #1e2a35; }
  .event:hover { background-color: #243440; }
  .sec-ago { font-family: monospace; color: #a1b0b3; }

  /* --- 🚦 Event Severity Colors (Dynamic Classes) --- */
  .row-info { border-left: 4px solid #3498db; }
  .row-warning {
    border-left: 4px solid #f39c12;
    background-image: linear-gradient(90deg, rgba(243, 156, 18, 0.1) 0%, transparent 100%);
  }
  .row-error {
    border-left: 4px solid #e74c3c;
    background-image: linear-gradient(90deg, rgba(231, 76, 60, 0.15) 0%, transparent 100%);
  }

  /* --- 📱 Style config (Mobile) --- */
  @media (max-width: 768px) {
    .ev-header { flex-direction: column; align-items: stretch; text-align: center; }
    .btn-clear { margin-top: 5px; padding: 12px; }

    /* 1. Hidden Header */
    .ev-header-row { display: none; }

    /* 2. Convert (Row) into Card (Dark Mode Style) */
    .event {
      display: block;
      border: 1px solid #34495e;
      border-radius: 8px;
      margin-bottom: 15px;
      padding: 10px;
      background: #1e2a35;
      box-shadow: 0 4px 10px rgba(0,0,0,0.3);
    }

    /* 3. Arrange each column on a separate line, leaving space for labels */
    .event div {
      position: relative;
      padding: 8px 0 8px 100px; /* Space for label on left */
      border-bottom: 1px dashed #2c3e50;
      text-align: right;
      word-break: break-word;
      color: #ecf0f1;
    }
    .event div:last-child { border-bottom: none; }

    /* FIX: Readable labels in Dark Mode */
    .event div::before {
      position: absolute;
      left: 5px;
      font-weight: bold;
      color: #bdc3c7; /* NEW READABLE LIGHT GRAY LABEL COLOR */
      text-align: left;
    }

    /* Set content (texts) for labels, leveraging nth-child on Divs */
    .event div:nth-child(1)::before { content: "📌 Type: "; }
    .event div:nth-child(2)::before { content: "🚨 Severity: "; }
    .event div:nth-child(3)::before { content: "🕒 Last Event: "; }
    .event div:nth-child(4)::before { content: "🔄 Count: "; }
    .event div:nth-child(5)::before { content: "🔢 Data: "; }
    .event div:nth-child(6)::before { content: "💬 Message: "; }
  }
</style>

<div class="card-events">
  <button class="btn-gray" onclick='goToMainPage()'>Back to main page</button>
  <div class="ev-header">
    <h3>⚠️ System Events & Alarms</h3>
    <button class="btn-clear" onclick="if(confirm('Are you sure you want to clear all events?')) window.location.href='/clearevents'">🗑️ Clear Events</button>
  </div>
  <div class="event-table">
    <div class="ev-header-row">
      <div>Event Type</div>
      <div>Severity</div>
      <div>Last Event</div>
      <div>Count</div>
      <div>Data</div>
      <div>Message</div>
    </div>
)=====";

const char EVENTS_HTML_END[] PROGMEM = R"=====(
  </div> </div> <style>
  /* 🦓 Zebra Striping: Alternating colors on the desktop for easier reading */
  @media (min-width: 769px) {
    .event:nth-child(even) { background-color: #1e2a35; }
    .event:nth-child(odd) { background-color: transparent; }
    .event.event-title { background-color: #2c3e50; }
  }
</style>

<script>
  // 🕒 Time calculation function: Converts milliseconds from the board to actual date (day/month/year) and time.
  function showEvent() {
    document.querySelectorAll(".event").forEach(function(e) {
      var n = e.querySelector(".sec-ago");
      if (n) {
        n.innerText = new Date(Number(BigInt(Date.now()) - BigInt(n.innerText))).toLocaleString();
      }
    });
  }

  // Run the time conversion function immediately after the website finishes loading.
  window.onload = function() { showEvent(); }

  function goToMainPage() { window.location.href = '/'; }

  // Auto-refresh every 5 seconds.
  setTimeout(function(){ location.reload(true); }, 5000);
</script>
)=====";

static std::vector<EventData> order_events;

void print_events_html(AsyncResponseStream* response) {
  response->print(FPSTR(EVENTS_HTML_START));

  order_events.clear();
  for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    const EVENTS_STRUCT_TYPE* event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
    if (event_pointer->occurences > 0) {
      order_events.push_back({static_cast<EVENTS_ENUM_TYPE>(i), event_pointer});
    }
  }

  std::sort(order_events.begin(), order_events.end(), compareEventsBySeverityAndTimestampDesc);
  uint64_t current_timestamp = millis64();

  // Stream data out each row
  for (const auto& event : order_events) {
    EVENTS_ENUM_TYPE event_handle = event.event_handle;
    const EVENTS_STRUCT_TYPE* event_pointer = event.event_pointer;

    String event_level = String(get_event_level_string(event_handle));
    String row_class = "row-info";

    // Blackground color change Logic (INFO=Green, WARNING=Orange, ERROR=Red)
    if (event_level == "ERROR") {
      row_class = "row-error";
    } else if (event_level == "WARNING") {
      row_class = "row-warning";
    }

    response->print("<div class='event " + row_class + "'>");

    response->print("<div>");
    response->print(get_event_enum_string(event_handle));
    response->print("</div>");

    response->print("<div>");
    response->print(event_level);
    response->print("</div>");

    response->print("<div class='sec-ago'>");
    response->print((unsigned long)(current_timestamp - event_pointer->timestamp));
    response->print("</div>");

    response->print("<div>");
    response->print(event_pointer->occurences);
    response->print("</div>");

    response->print("<div>");
    response->print(event_pointer->data);
    response->print("</div>");

    response->print("<div>");
    response->print(get_event_message_string(event_handle));
    response->print("</div>");

    response->print("</div>");
  }

  order_events.clear();
  response->print(FPSTR(EVENTS_HTML_END));
}