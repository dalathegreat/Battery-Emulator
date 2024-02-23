#include "cellmonitor_html.h"
#include <Arduino.h>

String cellmonitor_processor(const String& var) {
  if (var == "PLACEHOLDER") {
    String content = "";
    // Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content += ".container { display: flex; flex-wrap: wrap; justify-content: space-around; }";
    content += ".cell { width: 48%; margin: 1%; padding: 10px; border: 1px solid white; text-align: center; }";
    content += ".low-voltage { color: red; }";              // Style for low voltage text
    content += ".voltage-values { margin-bottom: 10px; }";  // Style for voltage values section
    content += "</style>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";

    // Display max, min, and deviation voltage values
    content += "<div class='voltage-values'>";
    content += "Max Voltage: " + String(system_cell_max_voltage_mV) + " mV<br>";
    content += "Min Voltage: " + String(system_cell_min_voltage_mV) + " mV<br>";
    int deviation = system_cell_max_voltage_mV - system_cell_min_voltage_mV;
    content += "Voltage Deviation: " + String(deviation) + " mV";
    content += "</div>";

    // Visualize the populated cells in forward order using flexbox with conditional text color
    content += "<div class='container'>";
    for (int i = 0; i < 120; ++i) {
      // Skip empty values
      if (system_cellvoltages_mV[i] == 0) {
        continue;
      }

      String cellContent = "Cell " + String(i + 1) + "<br>" + String(system_cellvoltages_mV[i]) + " mV";

      // Check if the cell voltage is below 3000, apply red color
      if (system_cellvoltages_mV[i] < 3000) {
        cellContent = "<span class='low-voltage'>" + cellContent + "</span>";
      }

      content += "<div class='cell'>" + cellContent + "</div>";
    }
    content += "</div>";

    // Close the block
    content += "</div>";

    content += "<button onclick='goToMainPage()'>Back to main page</button>";
    content += "<script>";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";
    return content;
  }
  return String();
}
