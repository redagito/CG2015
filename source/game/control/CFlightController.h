#pragma once
#include "game/IController.h"
#include "game/arena/ArenaTuning.h"
#include "game/flight/FlightState.h"
#include "game/flight/FlightTuning.h"
#include <optional>
class IInputProvider;

class CFlightController : public IController
{
  public:
    CFlightController(IInputProvider &input, const FlightTuning &tuning,
                      const ArenaTuning *arena = nullptr);
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
    std::optional<ArenaTuning> m_arena;
    CGameObject *m_object = nullptr;
    bool m_active = true;
};
