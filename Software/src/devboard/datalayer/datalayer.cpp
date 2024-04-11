#include "datalayer.h"
#include "../../include.h"

DataLayer datalayer;

DataLayer& datalayer_get_ref(void) {
  return datalayer;
}
