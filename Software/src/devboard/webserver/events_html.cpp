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
<div class="event-log">
<div class="event">
<div>Event Type</div><div>Severity</div><div>Last Event</div><div>Count</div><div>Data</div><div>Message</div>
</div>
)=====";
const char EVENTS_HTML_END[] = R"=====(
</div></div>
<button onclick='goToMainPage()'>Back to main page</button>
<script>function displayEventLog(){document.querySelector(".event-log");var i=(new Date).getTime()/1e3;document.querySelectorAll(".event").forEach(function(e){var n=e.querySelector(".last-event-seconds-ago"),t=e.querySelector(".timestamp");if(n&&t){var o=parseInt(n.innerText,10),a=parseFloat(t.innerText),r=new Date(1e3*(i-a+o)).toLocaleString();n.innerText=r}})}function goToMainPage(){window.location.href="/"}window.onload=function(){displayEventLog()}</script>
)=====";

/* The above <script> section is minified to save storage and increase performance, here is the full function: 
<script>
function displayEventLog() {
    var eventLogElement = document.querySelector('.event-log');

    // Get the current time on the client side
    var currentTime = new Date().getTime() / 1000; // Convert milliseconds to seconds

    // Loop through the events and update the "Last Event" column
    var events = document.querySelectorAll('.event');
    events.forEach(function(event) {
        var secondsAgoElement = event.querySelector('.last-event-seconds-ago');
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

// Call the displayEventLog function when the page is loaded
window.onload = function() {
    displayEventLog();
};

function goToMainPage() {
    window.location.href = '/';
}
</script>
*/

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
        content.concat("<div class='last-event-seconds-ago'>" + String((millis() / 1000) - entries[i].timestamp) +
                       "</div>");
        content.concat("<div>" + String(entries[i].occurences) + "</div>");
        content.concat("<div>" + String(entries[i].data) + "</div>");
        content.concat("<div>" + String(get_event_message(static_cast<EVENTS_ENUM_TYPE>(i))) + "</div>");
        content.concat("<div class='timestamp'>" + String(entries[i].timestamp) + "</div>");
        content.concat("</div>");  // End of event row
      }
    }
    content.concat(FPSTR(EVENTS_HTML_END));
    return content;
  }
  return String();
}
