#include "events_html.h"

const char EVENTS_HTML_START[] = R"=====(
<style>body{background-color:#000;color:#fff}.event-log{display:flex;flex-direction:column}.event{display:flex;flex-wrap:wrap;border:1px solid #fff;padding:10px}.event>div{flex:1;min-width:100px;max-width:90%;word-break:break-word}</style><div style="background-color:#303e47;padding:10px;margin-bottom:10px;border-radius:25px"><div class="event-log"><div class="event" style="background-color:#1e2c33;font-weight:700"><div>Event Type</div><div>Severity</div><div>Last Event</div><div>Count</div><div>Data</div><div>Message</div></div>
)=====";
const char EVENTS_HTML_END[] = R"=====(
</div></div>
<button onclick='home()'>Back to main page</button>
<style>.event:nth-child(even){background-color:#455a64}.event:nth-child(odd){background-color:#394b52}</style>
<script>function showEvent(){document.querySelectorAll(".event").forEach(function(e){var n=e.querySelector(".sec-ago");n&&(n.innerText=new Date(Date.now()-((+n.innerText.split(';')[0])*4294967296+ +n.innerText.split(';')[1])).toLocaleString());})}function home(){window.location.href="/"}window.onload=function(){showEvent()}</script>
)=====";

static std::vector<EventData> order_events;

String events_processor(const String& var) {
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
    std::sort(order_events.begin(), order_events.end(), compareEventsByTimestamp);
    unsigned long timestamp_now = millis();

    // Generate HTML and debug output
    for (const auto& event : order_events) {
      EVENTS_ENUM_TYPE event_handle = event.event_handle;
      event_pointer = event.event_pointer;
#ifdef DEBUG_VIA_USB
      Serial.println("Event: " + String(get_event_enum_string(event_handle)) +
                     " count: " + String(event_pointer->occurences) + " seconds: " + String(event_pointer->timestamp) +
                     " data: " + String(event_pointer->data) +
                     " level: " + String(get_event_level_string(event_handle)));
#endif
      content.concat("<div class='event'>");
      content.concat("<div>" + String(get_event_enum_string(event_handle)) + "</div>");
      content.concat("<div>" + String(get_event_level_string(event_handle)) + "</div>");
      content.concat("<div class='sec-ago'>" + String(millisrolloverCount) + ";" +
                     String(timestamp_now - event_pointer->timestamp) + "</div>");
      content.concat("<div>" + String(event_pointer->occurences) + "</div>");
      content.concat("<div>" + String(event_pointer->data) + "</div>");
      content.concat("<div>" + String(get_event_message_string(event_handle)) + "</div>");
      content.concat("</div>");  // End of event row
    }

    //clear the vector
    order_events.clear();
    content.concat(FPSTR(EVENTS_HTML_END));
    return content;
    return String();
  }
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
        var secondsAgoElement = event.querySelector('.sec-ago');
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
