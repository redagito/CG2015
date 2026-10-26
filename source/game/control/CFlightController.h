#pragma once
#include "game/IController.h"
#include "game/flight/FlightState.h"
#include "game/flight/FlightTuning.h"
class IInputProvider;

class CFlightController : public IController
{
  public:
    CFlightController(IInputProvider &input, const FlightTuning &tuning);
    void attach(CGameObject *object) override;
    void detach() override;
    void update(float seconds) override;
    void setActive(bool active) override;
    void receiveMessage(Message message) override;
    const FlightState &state() const;

  private:
    IInputProvider &m_input;
    FlightTuning m_tuning;
    FlightState m_state;
    CGameObject *m_object = nullptr;
    bool m_active = true;
};
