#include "events_html.h"
#include <limits>
#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/logging.h"
#include "../../devboard/utils/millis64.h"
#include "../i18n/tr.h"
#include "html_escape.h"
#include "index_html.h"

const char EVENTS_HTML_START[] = R"=====(
<style>body{background-color:#000;color:#fff}.event-log{display:flex;flex-direction:column}.event{display:flex;flex-wrap:wrap;border:1px solid #fff;padding:10px}.event>div{flex:1;min-width:100px;word-break:break-word}</style><div style="background-color:#303e47;padding:10px;margin-bottom:10px;border-radius:25px"><div class="event-log"><div class="event" style="background-color:#1e2c33;font-weight:700"><div>Event Type</div><div>Severity</div><div>Last Event</div><div>Count</div><div>Data</div><div>Message</div></div>
)=====";
const char EVENTS_HTML_BUTTONS[] = R"=====(
</div></div>
<style> button { background-color: #505E67; color: white; border: none; padding: 10px 20px; margin-bottom: 20px; cursor: pointer; border-radius: 10px; }
button:hover { background-color: #3A4A52; }</style>
)=====";
const char EVENTS_HTML_END[] = R"=====(
<style>.event:nth-child(even){background-color:#455a64}.event:nth-child(odd){background-color:#394b52}</style>
<script>function showEvent(){document.querySelectorAll(".event").forEach(function(e){var n=e.querySelector(".sec-ago");n&&(n.innerText=new Date(Number(BigInt(Date.now()) - BigInt(n.innerText))).toLocaleString())})}function home(){window.location.href="/"}window.onload=function(){showEvent()}
</script>
)=====";

static std::vector<EventData> order_events;

String events_processor(const String& var) {
  // COMMON_JAVASCRIPT is part of every template this serves; resolve its
  // placeholder here or the raw token ships to the browser.
  String common = common_javascript_processor(var);
  if (common.length() > 0) {
    return common;
  }

  if (var == "X") {
    String content = "";
    content.reserve(5000);
    // Page format
    content.concat(FPSTR(EVENTS_HTML_START));
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
      // Event text is served raw to MQTT, so it is escaped here at the point
      // it enters markup
      content.concat("<div>" + html_escape(get_event_message_string(event_handle)) + "</div>");
      content.concat("</div>");  // End of event row
    }

    //Script for refreshing page
    content += "<script>";
    content += "setTimeout(function(){ location.reload(true); }, 5000);";
    content += "</script>";

    //clear the vector
    order_events.clear();
    content.concat(FPSTR(EVENTS_HTML_BUTTONS));
    content += "<button onclick=\"askClear()\">" + TR(TrKey::UI_CLEAR_ALL_EVENTS) + "</button>";
    content += "<button onclick=\"home()\">" + TR(TrKey::UI_BACK_MAIN_PAGE) + "</button>";
    /* askClear() is built here rather than inside EVENTS_HTML_END: that raw
       literal is part of the string this processor RETURNS, not part of the
       template it is substituted into, so a %PLACEHOLDER% in it would never
       be resolved. */
    content += "<script>function askClear(){window.confirm('" + TR_JS(TrKey::UI_CONFIRM_CLEAR_ALL_EVENTS) +
               "')&&(window.location.href=\"/clearevents\")}</script>";
    content.concat(FPSTR(EVENTS_HTML_END));
    return content;
  }
  return String();
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
    function showEvent() {
        document.querySelectorAll(".event").forEach(function (e) {
            var n = e.querySelector(".sec-ago");
            n && (n.innerText = new Date(Number(BigInt(Date.now()) - BigInt(n.innerText))).toLocaleString());
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
    window.onload = function () {
        showEvent();
    };
</script>
*/
