#include "events_html.h"
#include <Arduino.h>
#include "../utils/events.h"

const char EVENTS_HTML_START[] = R"=====(
<style>body{background-color:#000;color:#fff}.event-log{display:flex;flex-direction:column}.event{display:flex;flex-wrap:wrap;border:1px solid #fff;padding:10px}.event>div{flex:1;min-width:100px;max-width:90%;word-break:break-word}</style><div style="background-color:#303e47;padding:10px;margin-bottom:10px;border-radius:25px"><div class="event-log"><div class="event" style="background-color:#1e2c33;font-weight:700"><div>Event Type</div><div>Severity</div><div>Last Event</div><div>Count</div><div>Data</div><div>Message</div></div>
)=====";
const char EVENTS_HTML_END[] = R"=====(
</div></div>
<button onclick='home()'>Back to main page</button>
<style>.event:nth-child(even){background-color:#455a64}.event:nth-child(odd){background-color:#394b52}</style>
<script>function showEvent(){document.querySelector(".event-log");var i=(new Date).getTime()/1e3;document.querySelectorAll(".event").forEach(function(e){var n=e.querySelector(".last-event-sec-ago"),t=e.querySelector(".timestamp");if(n&&t){var o=parseInt(n.innerText,10),a=parseFloat(t.innerText),r=new Date(1e3*(i-a+o)).toLocaleString();n.innerText=r}})}function home(){window.location.href="/"}window.onload=function(){showEvent()}</script>
)=====";

String events_processor(const String& var) {
  if (var == "ABC") {
    String content = "";
    content.reserve(5000);
    // Page format
    content.concat(FPSTR(EVENTS_HTML_START));
    const EVENTS_STRUCT_TYPE* event_pointer;
    for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
      event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
      EVENTS_ENUM_TYPE event_handle = static_cast<EVENTS_ENUM_TYPE>(i);
#ifdef DEBUG_VIA_USB
      Serial.println("Event: " + String(get_event_enum_string(event_handle)) +
                     " count: " + String(event_pointer->occurences) + " seconds: " + String(event_pointer->timestamp) +
                     " data: " + String(event_pointer->data) +
                     " level: " + String(get_event_level_string(event_handle)));
#endif
      if (event_pointer->occurences > 0) {
        content.concat("<div class='event'>");
        content.concat("<div>" + String(get_event_enum_string(event_handle)) + "</div>");
        content.concat("<div>" + String(get_event_level_string(event_handle)) + "</div>");
        content.concat("<div class='last-event-sec-ago'>" + String(event_pointer->timestamp) + "</div>");
        content.concat("<div>" + String(event_pointer->occurences) + "</div>");
        content.concat("<div>" + String(event_pointer->data) + "</div>");
        content.concat("<div>" + String(get_event_message_string(event_handle)) + "</div>");
        content.concat("<div class='timestamp' style='display:none;'>" + String(millis() / 1000) + "</div>");
        content.concat("</div>");  // End of event row
      }
    }
    content.concat(FPSTR(EVENTS_HTML_END));
    return content;
  }
  return String();
}

/* Script for displaying event log before it gets minified
<script>
function showEvent() {
    var eventLogElement = document.querySelector('.event-log');
    // Get the current time on the client side
    var currentTime = new Date().getTime() / 1000; // Convert milliseconds to seconds
    // Loop through the events and update the "Last Event" column
    var events = document.querySelectorAll('.event');
    events.forEach(function(event) {
        var secondsAgoElement = event.querySelector('.last-event-sec-ago');
        var timestampElement = event.querySelector('.timestamp');
        if (secondsAgoElement && timestampElement) {
            var secondsAgo = parseInt(secondsAgoElement.innerText, 10);
            var uptimeTimestamp = parseFloat(timestampElement.innerText); // Parse as float to handle seconds with decimal parts
            // Calculate the actual system time based on the client-side current time
            var actualTime = new Date((currentTime - uptimeTimestamp + secondsAgo) * 1000);
            // Format the date and time
            var formattedTime = actualTime.toLocaleString();
            // Update the "Last Event" column with the formatted time
            secondsAgoElement.innerText = formattedTime;
        }
    });
}

// Call the showEvent function when the page is loaded
window.onload = function() {
    showEvent();
};

function home() {
    window.location.href = '/';
}
</script>
*/
