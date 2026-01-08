/**
 * @file coherence_protocol.cpp
 * @brief Factory implementation for coherence protocols
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 */

#include "coherence_protocol.h"
#include "mesi_protocol.h"
#include "moesi_protocol.h"
#include "msi_protocol.h"

namespace cachesim {

/**
 * @brief Adapter to wrap MESIProtocol into CoherenceProtocolBase interface
 */
class MESIProtocolAdapter : public CoherenceProtocolBase {
public:
  MESIProtocolAdapter() = default;

  int handleLocalRead(int currentState, bool sharedCopy) override {
    return static_cast<int>(protocol_.handleLocalRead(
        static_cast<MESIState>(currentState), sharedCopy));
  }

  int handleLocalWrite(int currentState) override {
    return static_cast<int>(
        protocol_.handleLocalWrite(static_cast<MESIState>(currentState)));
  }

  int handleRemoteRead(int currentState) override {
    return static_cast<int>(
        protocol_.handleRemoteRead(static_cast<MESIState>(currentState)));
  }

  int handleRemoteWrite(int currentState) override {
    return static_cast<int>(
        protocol_.handleRemoteWrite(static_cast<MESIState>(currentState)));
  }

  [[nodiscard]] bool requiresWriteback(int state) const override {
    return protocol_.requiresWriteback(static_cast<MESIState>(state));
  }

  [[nodiscard]] bool isValid(int state) const override {
    return protocol_.isValid(static_cast<MESIState>(state));
  }

  [[nodiscard]] std::string_view stateName(int state) const override {
    return protocol_.stateToString(static_cast<MESIState>(state));
  }

  [[nodiscard]] std::string_view protocolName() const override {
    return "MESI";
  }

  [[nodiscard]] int invalidState() const override {
    return static_cast<int>(MESIState::Invalid);
  }

private:
  MESIProtocol protocol_;
};

std::unique_ptr<CoherenceProtocolBase>
CoherenceProtocolBase::create(CoherenceProtocolType type) {
  switch (type) {
  case CoherenceProtocolType::MSI:
    return std::make_unique<MSIProtocol>();

  case CoherenceProtocolType::MESI:
    return std::make_unique<MESIProtocolAdapter>();

  case CoherenceProtocolType::MOESI:
    return std::make_unique<MOESIProtocol>();

  default:
    // Default to MESI
    return std::make_unique<MESIProtocolAdapter>();
  }
}

} // namespace cachesim
