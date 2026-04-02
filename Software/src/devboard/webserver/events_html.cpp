#include "events_html.h"
#include <limits>
#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/logging.h"
#include "../../devboard/utils/millis64.h"
#include "index_html.h"

const char EVENTS_HTML_START[] PROGMEM = R"=====(
<style>
  /* --- 📦 Container & Header --- */
  .card-events { background: #fff; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.05); padding: 20px; border-top: 4px solid #e74c3c; margin-bottom: 20px; }
  .ev-header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #eee; padding-bottom: 15px; margin-bottom: 15px; flex-wrap: wrap; gap: 10px; }
  .ev-header h3 { margin: 0; color: #333; font-size: 1.25rem; }
  .btn-clear { background: #e74c3c; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; cursor: pointer; transition: 0.2s; }
  .btn-clear:hover { background: #c0392b; }

  /* --- 💻 Desktop Grid Layout --- */
  .event-table { display: block; border-collapse: collapse; }
  .ev-header-row {
	display: grid;
	grid-template-columns: 2fr 1fr 1.5fr 0.8fr 1fr 3fr;
	background-color: #3498db;
	color: white;
	padding: 12px 15px;
	font-weight: bold;
	border-radius: 6px 6px 0 0; }
  .event {
	display: grid;
    grid-template-columns: 2fr 1fr 1.5fr 0.8fr 1fr 3fr; /* Locks column alignment to prevent overlapping */
	padding: 12px 15px;
	border-bottom: 1px solid #ddd;
	align-items: center; }
  .event:nth-child(even) { background-color: #f9f9f9; }
  .event:hover { background-color: #f1f1f1; }
  .sec-ago { font-family: monospace; color: #555; }

  /* --- Style config (Mobile) --- */
  @media (max-width: 768px) {
    .ev-header { flex-direction: column; align-items: stretch; text-align: center; }
    .btn-clear { margin-top: 5px; padding: 12px; }

    /* 1. Hidden Header */
    .ev-header-row { display: none; }

    /* 2. Convert (Row) into Card */
    .event {
      display: block;
      border: 1px solid #ccc;
      border-radius: 8px;
      margin-bottom: 15px;
      padding: 10px;
      background: #fff;
      box-shadow: 0 2px 5px rgba(0,0,0,0.05);
    }

    /* 3. Arrange each column on a separate line, leaving space for labels */
    .event div {
      position: relative;
      padding: 8px 0 8px 100px;
      border-bottom: 1px dashed #eee;
      text-align: right;
      word-break: break-word;
    }
    .event div:last-child { border-bottom: none; }

    .event div::before {
      position: absolute;
      left: 5px;
      font-weight: bold;
      color: #3498db;
      text-align: left;
    }

    .event div:nth-child(1)::before { content: "📌 Type: "; font-weight: bold; color: #7f8c8d; }
    .event div:nth-child(2)::before { content: "🚨 Severity: "; font-weight: bold; color: #7f8c8d; }
    .event div:nth-child(3)::before { content: "🕒 Last Event: "; font-weight: bold; color: #7f8c8d; }
    .event div:nth-child(4)::before { content: "🔄 Count: "; font-weight: bold; color: #7f8c8d; }
    .event div:nth-child(5)::before { content: "🔢 Data: "; font-weight: bold; color: #7f8c8d; }
    .event div:nth-child(6)::before { content: "💬 Message: "; font-weight: bold; color: #7f8c8d; }
  }
</style>

<div class="card-events">
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
    .event:nth-child(even) { background-color: #f9f9f9; }
    .event:nth-child(odd) { background-color: #ffffff; }
    .event.event-title { background-color: #f0f0f0; }
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
    String bg_style = "";
    String text_color = "#000000";

    // Blackground color change Logic (INFO=Green, WARNING=Orange, ERROR=Red)
    if (event_level == "ERROR") {
      bg_style = "linear-gradient(180deg,rgba(250, 5, 5, 1) 25%, rgba(252, 252, 252, 1) 100%)";
      text_color = "#ffffff";
    } else {
      bg_style = "linear-gradient(180deg,rgba(245, 255, 240, 1) 0%, rgba(252, 252, 252, 1) 100%)";
    }

    if (bg_style.length() > 0) {
      response->print("<div class='event' style='background: ");
      response->print(bg_style);
      response->print("; color: ");
      response->print(text_color);
      response->print(";'>");
    } else {
      response->print("<div class='event'>");
    }

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

    response->print("</div>");  // ปิด div Row
  }

  order_events.clear();
  response->print(FPSTR(EVENTS_HTML_END));
}