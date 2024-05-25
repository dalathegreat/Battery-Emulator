#include "cellmonitor_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"

String cellmonitor_processor(const String& var) {
  if (var == "X") {
    String content = "";
    // Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content += ".container { display: flex; flex-wrap: wrap; justify-content: space-around; }";
    content += ".cell { width: 48%; margin: 1%; padding: 10px; border: 1px solid white; text-align: center; }";
    content += ".low-voltage { color: red; }";              // Style for low voltage text
    content += ".voltage-values { margin-bottom: 10px; }";  // Style for voltage values section
    content += "#graph {display: flex;align-items: flex-end;height: 200px;border: 1px solid #ccc;position: relative;}";
    content +=
        ".bar {margin: 0 0px;background-color: blue;display: inline-block;position: relative;cursor: pointer;border: "
        "1px solid white; /* Add this line */}";
    content += "#valueDisplay {text-align: left;font-weight: bold;margin-top: 10px;}";
    content += "</style>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";

    // Display max, min, and deviation voltage values
    content += "<div id='voltageValues' class='voltage-values'></div>";
    // Display cells
    content += "<div id='cellContainer' class='container'></div>";
    // Display bars
    content += "<div id='graph'></div>";
    // Display single hovered value
    content += "<div id='valueDisplay'>Value: ...</div>";

    // Close the block
    content += "</div>";
    content += "<button onclick='home()'>Back to main page</button>";
    content += "<script>";
    // Populate cell data
    content += "const data = [";
    for (uint8_t i = 0u; i < MAX_AMOUNT_CELLS; i++) {
      if (datalayer.battery.status.cell_voltages_mV[i] == 0) {
        continue;
      }
      content += String(datalayer.battery.status.cell_voltages_mV[i]) + ",";
    }
    content += "];";

    content += "const min_mv = Math.min(...data) - 20;";
    content += "const max_mv = Math.max(...data) + 20;";
    content += "const min_index = data.indexOf(Math.min(...data));";
    content += "const max_index = data.indexOf(Math.max(...data));";
    content += "const graphContainer = document.getElementById('graph');";
    content += "const valueDisplay = document.getElementById('valueDisplay');";
    content += "const cellContainer = document.getElementById('cellContainer');";

    content += "function home() { window.location.href = '/'; }";

    // Arduino-style map() function
    content +=
        "function map(value, fromLow, fromHigh, toLow, toHigh) {return (value - fromLow) * (toHigh - toLow) / "
        "(fromHigh - fromLow) + toLow;}";

    // Mark cell and bar with highest/lowest values
    content +=
        "function checkMinMax(cell, bar, index) {if ((index == min_index) || (index == max_index)) "
        "{cell.style.borderColor = 'red';bar.style.borderColor = 'red';}}";

    // Bar function. Basically get the mV, scale the height and add a bar div to its container
    content +=
        "function createBars(data) {"
        "data.forEach((mV, index) => {"
        "const bar = document.createElement('div');"
        "const mV_limited = map(mV, min_mv, max_mv, 20, 200);"
        "bar.className = 'bar';"
        "bar.id = `barIndex${index}`;"
        "bar.style.height = `${mV_limited}px`;"
        "bar.style.width = `${750/data.length}px`;"

        "const cell = document.getElementById(`cellIndex${index}`);"

        "checkMinMax(cell, bar, index);"

        "bar.addEventListener('mouseenter', () => {"
        "valueDisplay.textContent = `Value: ${mV}`;"
        "bar.style.backgroundColor = `lightblue`;"
        "cell.style.backgroundColor = `blue`;"
        "});"

        "bar.addEventListener('mouseleave', () => {"
        "valueDisplay.textContent = 'Value: ...';"
        "bar.style.backgroundColor = `blue`;"
        "cell.style.removeProperty('background-color');"
        "});"

        "graphContainer.appendChild(bar);"
        "});"
        "}";

    // Cell population function. For each value, add a cell block with its value
    content +=
        "function createCells(data) {"
        "data.forEach((mV, index) => {"
        "const cell = document.createElement('div');"
        "cell.className = 'cell';"
        "cell.id = `cellIndex${index}`;"
        "let cellContent = `Cell ${index + 1}<br>${mV} mV`;"
        "if (mV < 3000) {"
        "cellContent = `<span class='low-voltage'>${cellContent}</span>`;"
        "}"
        "cell.innerHTML = cellContent;"

        "cell.addEventListener('mouseenter', () => {"
        "let bar = document.getElementById(`barIndex${index}`);"
        "valueDisplay.textContent = `Value: ${mV}`;"
        "bar.style.backgroundColor = `lightblue`;"
        "cell.style.backgroundColor = `blue`;"
        "});"

        "cell.addEventListener('mouseleave', () => {"
        "let bar = document.getElementById(`barIndex${index}`);"
        "bar.style.backgroundColor = `blue`;"
        "cell.style.removeProperty('background-color');"
        "});"

        "cellContainer.appendChild(cell);"
        "});"
        "}";

    // On fetch, update the header of max/min/deviation client-side for consistency
    content +=
        "function updateVoltageValues(data) {"
        "const min_mv = Math.min(...data);"
        "const max_mv = Math.max(...data);"
        "const cell_dev = max_mv - min_mv;"
        "const voltVal = document.getElementById('voltageValues');"
        "voltVal.innerHTML = `Max Voltage : ${max_mv} mV<br>Min Voltage: ${min_mv} mV<br>Voltage Deviation: "
        "${cell_dev} mV`"
        "}";

    // If we have values, do the thing. Otherwise, display friendly message and wait
    content += "if (data.length != 0) {";
    content += "createCells(data);";
    content += "createBars(data);";
    content += "updateVoltageValues(data);";
    content += "}";
    content += "else {";
    content +=
        "document.getElementById('voltageValues').textContent = 'Cell information not yet fetched, or information not "
        "available';";
    content += "}";

    // Automatic refresh is nice
    content += "setTimeout(function(){ location.reload(true); }, 10000);";

    content += "</script>";
    return content;
  }
  return String();
}
