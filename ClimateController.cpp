#include "ClimateController.h"

#include "ClimClient.h"
#include "Log.h"
#include "RuntimeConfig.h"

ClimateController::ClimateController(RuntimeConfig& cfg, ClimClient& climClient, float hystC)
  : _cfg(cfg), _clim(climClient), _hystC(hystC) {}

void ClimateController::tick(const DateTime& now, float roomTempC) {
  if (isnan(roomTempC)) return;

  if (!_cfg.isWithinSchedule(now)) {
    if (_isOn) {
      _clim.setOff();
      _isOn = false;
      logInfo("CLIM", "OFF (out of schedule)");
    }
    return;
  }

  float minC = _cfg.tempMinC();
  float maxC = _cfg.tempMaxC();

  if (roomTempC > maxC + _hystC) {
    _clim.setTemp(maxC);
    if (!_isOn) {
      _clim.setOn();
      _isOn = true;
      logInfo("CLIM", String("ON target=") + maxC + "C (temp high)");
    }
    return;
  }

  if (roomTempC < minC - _hystC) {
    _clim.setTemp(minC);
    if (!_isOn) {
      _clim.setOn();
      _isOn = true;
      logInfo("CLIM", String("ON target=") + minC + "C (temp low)");
    }
    return;
  }

  if (_isOn && roomTempC > minC + _hystC && roomTempC < maxC - _hystC) {
    _clim.setOff();
    _isOn = false;
    logInfo("CLIM", "OFF (temp ok)");
  }
}
