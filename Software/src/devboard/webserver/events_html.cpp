#include "events_html.h"
#include <Arduino.h>

const char EVENTS_HTML_START[] = R"=====(
<style>
    body { background-color: black; color: white; }
    .event-log { display: flex; flex-direction: column; }
    .event { display: flex; flex-wrap: wrap; border: 1px solid white; padding: 10px; }
    .event > div { flex: 1; min-width: 100px; max-width: 90%; word-break: break-word; }
</style>
<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>
<h4 style='color: white;'>Event log:</h4>
<div class="event-log">
<div class="event">
<div>Event Type</div><div>LED Color</div><div>Last Event (seconds ago)</div><div>Count</div><div>Data</div><div>Message</div>
</div>
)=====";
const char EVENTS_HTML_END[] = R"=====(
</div></div>
<button onclick='goToMainPage()'>Back to main page</button>
<script>
function goToMainPage() {
    window.location.href = '/';
}
</script>
)=====";

String events_processor(const String& var) {
  if (var == "PLACEHOLDER") {
    String content = "";
    content.reserve(5000);
    // Page format
    content.concat(FPSTR(EVENTS_HTML_START));
    for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
      Serial.println("Event: " + String(get_event_enum_string(static_cast<EVENTS_ENUM_TYPE>(i))) +
                     " count: " + String(entries[i].occurences) + " seconds: " + String(entries[i].timestamp) +
                     " data: " + String(entries[i].data));
      if (entries[i].occurences > 0) {
        content.concat("<div class='event'>");
        content.concat("<div>" + String(get_event_enum_string(static_cast<EVENTS_ENUM_TYPE>(i))) + "</div>");
        content.concat("<div>" + String(get_led_color_display_text(entries[i].led_color)) + "</div>");
        content.concat("<div>" + String((millis() / 1000) - entries[i].timestamp) + "</div>");
        content.concat("<div>" + String(entries[i].occurences) + "</div>");
        content.concat("<div>" + String(entries[i].data) + "</div>");
        content.concat("<div>" + String(get_event_message(static_cast<EVENTS_ENUM_TYPE>(i))) + "</div>");
        content.concat("</div>");  // End of event row
      }
    }
    content.concat(FPSTR(EVENTS_HTML_END));
    return content;
  }
  return String();
}
