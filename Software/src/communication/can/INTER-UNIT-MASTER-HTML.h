#ifndef _INTER_UNIT_MASTER_HTML_H_
#define _INTER_UNIT_MASTER_HTML_H_

#include "../../devboard/webserver/BatteryHtmlRenderer.h"

class InterUnitMasterHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif  // _INTER_UNIT_MASTER_HTML_H_
