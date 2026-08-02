#ifndef _RIVIAN_HTML_H
#define _RIVIAN_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class RivianHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
