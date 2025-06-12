#include "cellmonitor_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"

String cellmonitor_processor(const String& var) {
  if (var == "X") {
    String content = "";
    // Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content +=
        "button { background-color: #505E67; color: white; border: none; padding: 10px 20px; margin-bottom: 20px; "
        "cursor: pointer; border-radius: 10px; }";
    content += "button:hover { background-color: #3A4A52; }";
    content += ".container { display: flex; flex-wrap: wrap; justify-content: space-around; }";
    content += ".cell { width: 48%; margin: 1%; padding: 10px; border: 1px solid white; text-align: center; }";
    content += ".low-voltage { color: red; }";              // Style for low voltage text
    content += ".voltage-values { margin-bottom: 10px; }";  // Style for voltage values section

    if (battery2) {
      content +=
          "#graph, #graph2 {display: flex;align-items: flex-end;height: 200px;border: 1px solid #ccc;position: "
          "relative;}";
    } else {
      content +=
          "#graph {display: flex;align-items: flex-end;height: 200px;border: 1px solid #ccc;position: relative;}";
    }
    content +=
        ".bar {margin: 0 0px;background-color: blue;display: inline-block;position: relative;cursor: pointer;border: "
        "1px solid white; /* Add this line */}";

    if (battery2) {
      content += "#valueDisplay, #valueDisplay2 {text-align: left;font-weight: bold;margin-top: 10px;}";
    } else {
      content += "#valueDisplay {text-align: left;font-weight: bold;margin-top: 10px;}";
    }
    content += "</style>";

    content += "<button onclick='home()'>Back to main page</button>";

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
    //Legend for graph
    content +=
        "<span style='color: white; background-color: blue; font-weight: bold; padding: 2px 8px; border-radius: 4px; "
        "margin-right: 15px;'>Idle</span>";
    bool battery_balancing = false;
    for (uint8_t i = 0u; i < datalayer.battery.info.number_of_cells; i++) {
      battery_balancing = datalayer.battery.status.cell_balancing_status[i];
      if (battery_balancing)
        break;
    }
    if (battery_balancing) {
      content +=
          "<span style='color: black; background-color: #00FFFF; font-weight: bold; padding: 2px 8px; border-radius: "
          "4px; margin-right: 15px;'>Balancing</span>";
    }
    content +=
        "<span style='color: white; background-color: red; font-weight: bold; padding: 2px 8px; border-radius: "
        "4px;'>Min/Max</span>";

    // Close the block
    content += "</div>";

    if (battery2) {
      // Start a new block with a specific background color
      content += "<div style='background-color: #303E41; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";

      // Display max, min, and deviation voltage values
      content += "<div id='voltageValues2' class='voltage-values'></div>";
      // Display cells
      content += "<div id='cellContainer2' class='container'></div>";
      // Display bars
      content += "<div id='graph2'></div>";
      // Display single hovered value
      content += "<div id='valueDisplay2'>Value: ...</div>";
      //Legend for graph
      content +=
          "<span style='color: white; background-color: blue; font-weight: bold; padding: 2px 8px; border-radius: 4px; "
          "margin-right: 15px;'>Idle</span>";

      bool battery2_balancing = false;
      for (uint8_t i = 0u; i < datalayer.battery2.info.number_of_cells; i++) {
        battery2_balancing = datalayer.battery2.status.cell_balancing_status[i];
        if (battery2_balancing)
          break;
      }
      if (battery2_balancing) {
        content +=
            "<span style='color: black; background-color: #00FFFF; font-weight: bold; padding: 2px 8px; border-radius: "
            "4px; margin-right: 15px;'>Balancing</span>";
      }
      content +=
          "<span style='color: white; background-color: red; font-weight: bold; padding: 2px 8px; border-radius: "
          "4px;'>Min/Max</span>";

      // Close the block
      content += "</div>";

      content += "<button onclick='home()'>Back to main page</button>";
    }

    content += "<script>";
    // Populate cell data
    content += "const data = [";
    for (uint8_t i = 0u; i < datalayer.battery.info.number_of_cells; i++) {
      if (datalayer.battery.status.cell_voltages_mV[i] == 0) {
        continue;
      }
      content += String(datalayer.battery.status.cell_voltages_mV[i]) + ",";
    }
    content += "];";

    content += "const balancing = [";
    for (uint8_t i = 0u; i < datalayer.battery.info.number_of_cells; i++) {
      if (datalayer.battery.status.cell_voltages_mV[i] == 0) {
        continue;
      }
      content += datalayer.battery.status.cell_balancing_status[i] ? "true," : "false,";
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
        "if (balancing[index]) {"
        "  bar.style.backgroundColor = '#00FFFF';"  // Cyan color for balancing
        "  bar.style.borderColor = '#00FFFF';"
        "} else {"
        "  bar.style.backgroundColor = 'blue';"  // Normal blue for non-balancing
        "  bar.style.borderColor = 'white';"
        "}"

        "const cell = document.getElementById(`cellIndex${index}`);"

        "checkMinMax(cell, bar, index);"

        "bar.addEventListener('mouseenter', () => {"
        "    valueDisplay.textContent = `Value: ${mV}` + (balancing[index] ? ' (balancing)' : '');"
        "    bar.style.backgroundColor = balancing[index] ? '#80FFFF' : 'lightblue';"
        "    cell.style.backgroundColor = balancing[index] ? '#006666' : 'blue';"
        "});"

        "bar.addEventListener('mouseleave', () => {"
        "valueDisplay.textContent = 'Value: ...';"
        "bar.style.backgroundColor = balancing[index] ? '#00FFFF' : 'blue';"  // Restore cyan if balancing, else blue
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
        "  cellContent = `<span class='low-voltage'>${cellContent}</span>`;"
        "}"
        "cell.innerHTML = cellContent;"

        "cell.addEventListener('mouseenter', () => {"
        "let bar = document.getElementById(`barIndex${index}`);"
        "valueDisplay.textContent = `Value: ${mV}`;"
        "bar.style.backgroundColor = balancing[index] ? '#80FFFF' : 'lightblue';"  // Lighter cyan if balancing
        "cell.style.backgroundColor = balancing[index] ? '#006666' : 'blue';"      // Darker cyan if balancing
        "});"

        "cell.addEventListener('mouseleave', () => {"
        "let bar = document.getElementById(`barIndex${index}`);"
        "bar.style.backgroundColor = balancing[index] ? '#00FFFF' : 'blue';"  // Restore original color
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

    if (battery2) {
      // Populate cell data
      content += "const data2 = [";
      for (uint8_t i = 0u; i < datalayer.battery2.info.number_of_cells; i++) {
        if (datalayer.battery2.status.cell_voltages_mV[i] == 0) {
          continue;
        }
        content += String(datalayer.battery2.status.cell_voltages_mV[i]) + ",";
      }
      content += "];";

      content += "const balancing2 = [";
      for (uint8_t i = 0u; i < datalayer.battery2.info.number_of_cells; i++) {
        if (datalayer.battery2.status.cell_voltages_mV[i] == 0) {
          continue;
        }
        content += datalayer.battery2.status.cell_balancing_status[i] ? "true," : "false,";
      }
      content += "];";

      content += "const min_mv2 = Math.min(...data2) - 20;";
      content += "const max_mv2 = Math.max(...data2) + 20;";
      content += "const min_index2 = data2.indexOf(Math.min(...data2));";
      content += "const max_index2 = data2.indexOf(Math.max(...data2));";
      content += "const graphContainer2 = document.getElementById('graph2');";
      content += "const valueDisplay2 = document.getElementById('valueDisplay2');";
      content += "const cellContainer2 = document.getElementById('cellContainer2');";

      // Arduino-style map() function
      content +=
          "function map2(value, fromLow, fromHigh, toLow, toHigh) {return (value - fromLow) * (toHigh - toLow) / "
          "(fromHigh - fromLow) + toLow;}";

      // Mark cell and bar with highest/lowest values
      content +=
          "function checkMinMax2(cell2, bar2, index2) {if ((index2 == min_index2) || (index2 == max_index2)) "
          "{cell2.style.borderColor = 'red';bar2.style.borderColor = 'red';}}";

      // Bar function. Basically get the mV, scale the height and add a bar div to its container
      content +=
          "function createBars2(data2) {"
          "data2.forEach((mV, index2) => {"
          "const bar2 = document.createElement('div');"
          "const mV_limited2 = map2(mV, min_mv2, max_mv2, 20, 200);"
          "bar2.className = 'bar';"
          "bar2.id = `barIndex2${index2}`;"
          "bar2.style.height = `${mV_limited2}px`;"
          "bar2.style.width = `${750/data2.length}px`;"
          "if (balancing2[index2]) {"
          "  bar2.style.backgroundColor = '#00FFFF';"  // Cyan color for balancing
          "  bar2.style.borderColor = '#00FFFF';"
          "} else {"
          "  bar2.style.backgroundColor = 'blue';"  // Normal blue for non-balancing
          "  bar2.style.borderColor = 'white';"
          "}"
          "const cell2 = document.getElementById(`cellIndex2${index2}`);"

          "checkMinMax2(cell2, bar2, index2);"

          "bar2.addEventListener('mouseenter', () => {"
          "    valueDisplay2.textContent = `Value: ${mV}` + (balancing[index2] ? ' (balancing)' : '');"
          "    bar2.style.backgroundColor = balancing2[index2] ? '#80FFFF' : 'lightblue';"
          "    cell2.style.backgroundColor = balancing2[index2] ? '#006666' : 'blue';"
          "});"

          "bar2.addEventListener('mouseleave', () => {"
          "valueDisplay2.textContent = 'Value: ...';"
          "bar2.style.backgroundColor = balancing2[index2] ? '#00FFFF' : 'blue';"  // Restore cyan if balancing, else blue
          "cell2.style.removeProperty('background-color');"
          "});"

          "graphContainer2.appendChild(bar2);"
          "});"
          "}";

      // Cell population function. For each value, add a cell block with its value
      content +=
          "function createCells2(data2) {"
          "data2.forEach((mV, index2) => {"
          "const cell2 = document.createElement('div');"
          "cell2.className = 'cell';"
          "cell2.id = `cellIndex2${index2}`;"
          "let cellContent2 = `Cell ${index2 + 1}<br>${mV} mV`;"
          "if (mV < 3000) {"
          "cellContent2 = `<span class='low-voltage'>${cellContent2}</span>`;"
          "}"
          "cell2.innerHTML = cellContent2;"

          "cell2.addEventListener('mouseenter', () => {"
          "let bar2 = document.getElementById(`barIndex2${index2}`);"
          "valueDisplay2.textContent = `Value: ${mV}`;"
          "bar2.style.backgroundColor = balancing2[index2] ? '#80FFFF' : 'lightblue';"  // Lighter cyan if balancing
          "cell2.style.backgroundColor = balancing2[index2] ? '#006666' : 'blue';"      // Darker cyan if balancing
          "});"

          "cell2.addEventListener('mouseleave', () => {"
          "let bar2 = document.getElementById(`barIndex2${index2}`);"
          "bar2.style.backgroundColor = balancing2[index2] ? '#00FFFF' : 'blue';"  // Restore original color
          "cell2.style.removeProperty('background-color');"
          "});"

          "cellContainer2.appendChild(cell2);"
          "});"
          "}";

      // On fetch, update the header of max/min/deviation client-side for consistency
      content +=
          "function updateVoltageValues2(data2) {"
          "const min_mv2 = Math.min(...data2);"
          "const max_mv2 = Math.max(...data2);"
          "const cell_dev2 = max_mv2 - min_mv2;"
          "const voltVal2 = document.getElementById('voltageValues2');"
          "voltVal2.innerHTML = `Battery #2<br>Max Voltage : ${max_mv2} mV<br>Min Voltage: ${min_mv2} mV<br>Voltage "
          "Deviation: "
          "${cell_dev2} mV`"
          "}";

      // If we have values, do the thing. Otherwise, display friendly message and wait
      content += "if (data2.length != 0) {";
      content += "createCells2(data2);";
      content += "createBars2(data2);";
      content += "updateVoltageValues2(data2);";
      content += "}";
      content += "else {";
      content +=
          "document.getElementById('voltageValues2').textContent = 'Cell information not yet fetched, or information "
          "not "
          "available';";
      content += "}";
    }

    // Automatic refresh is nice
    content += "setTimeout(function(){ location.reload(true); }, 20000);";

    content += "</script>";
    return content;
  }
  return String();
}
