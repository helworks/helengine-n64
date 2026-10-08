#pragma once

#include <cstddef>
#include <cstdint>

#include "IRuntimeCpuProfileSink.hpp"
#include "platform/n64/Nintendo64CpuProfileWindow.hpp"

namespace helengine::n64 {

/// Bridges generated Core probes to the bounded, periodically reported N64 profile window.
class Nintendo64CpuProfileSink final : public ::IRuntimeCpuProfileSink {
public:
    enum NativeStageId : std::uint32_t {
        HostUpdate = 100u,
        RenderManager2D = 101u,
        RdpqDetachWait = 102u
    };

    std::uint64_t ReadMicroseconds() override;
    void RecordStage(::RuntimeCpuProfileStage stage, std::uint64_t elapsedMicroseconds) override;
    void RecordUpdateable(::IUpdateable* item, std::int32_t index, std::uint32_t typeHash,
                          std::uint32_t ownerSceneEntityId, std::uint64_t elapsedMicroseconds) override;
    void RegisterType(::IUpdateable* item, std::uint32_t typeHash, HeCppString typeName) override;
    void ClockFault() override;

    void BeginFrame(std::uint64_t sequence);
    void EndFrame(std::uint64_t sequence);
    void RecordNativeStage(std::uint32_t stageId, std::uint64_t elapsedMicroseconds);
    void EmitPendingReport();
    bool HasPendingReport() const { return Window.IsReportReady(); }

private:
    struct TypeRegistration {
        std::uintptr_t Identity = 0u;
        std::uint32_t TypeHash = 0u;
        char Name[48]{};
    };
    static constexpr std::size_t MaxTypeRegistrations = 64u;
    const TypeRegistration* FindTypeRegistration(std::uintptr_t identity) const;
    const char* FindTypeName(std::uintptr_t identity, std::uint32_t typeHash) const;

    Nintendo64CpuProfileWindow Window;
    TypeRegistration TypeRegistrations[MaxTypeRegistrations]{};
    std::size_t TypeRegistrationCount = 0u;
};

} // namespace helengine::n64
