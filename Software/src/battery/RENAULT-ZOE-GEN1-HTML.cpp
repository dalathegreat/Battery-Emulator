#include "RENAULT-ZOE-GEN1-HTML.h"
#include "../datalayer/datalayer_extended.h"

String RenaultZoeGen1HtmlRenderer::get_status_html() {
  String content;

  content += "<h4>CUV " + String(datalayer_extended.zoe.CUV) + "</h4>";
  content += "<h4>HVBIR " + String(datalayer_extended.zoe.HVBIR) + "</h4>";
  content += "<h4>HVBUV " + String(datalayer_extended.zoe.HVBUV) + "</h4>";
  content += "<h4>EOCR " + String(datalayer_extended.zoe.EOCR) + "</h4>";
  content += "<h4>HVBOC " + String(datalayer_extended.zoe.HVBOC) + "</h4>";
  content += "<h4>HVBOT " + String(datalayer_extended.zoe.HVBOT) + "</h4>";
  content += "<h4>HVBOV " + String(datalayer_extended.zoe.HVBOV) + "</h4>";
  content += "<h4>COV " + String(datalayer_extended.zoe.COV) + "</h4>";
  content += "<h4>Battery mileage " + String(datalayer_extended.zoe.mileage_km) + " km</h4>";
  content += "<h4>Alltime energy " + String(datalayer_extended.zoe.alltime_kWh) + " kWh</h4>";
  return content;
}
