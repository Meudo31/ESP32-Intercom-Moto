#include "system_events.h"

static EventGroupHandle_t s_groupe = nullptr;

void sysInit() { if (!s_groupe) s_groupe = xEventGroupCreate(); }
void sysSet(EventBits_t bits) { xEventGroupSetBits(s_groupe, bits); }
void sysClear(EventBits_t bits) { xEventGroupClearBits(s_groupe, bits); }
EventBits_t sysGet() { return xEventGroupGetBits(s_groupe); }
bool sysIs(EventBits_t bits) { return (sysGet() & bits) == bits; }
