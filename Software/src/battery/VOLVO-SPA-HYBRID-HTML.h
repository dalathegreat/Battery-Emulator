#ifndef _VOLVO_SPA_HYBRID_HTML_H
#define _VOLVO_SPA_HYBRID_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class VolvoSpaHybridHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
