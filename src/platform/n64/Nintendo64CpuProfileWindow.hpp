#pragma once

#include <cstddef>
#include <cstdint>

namespace helengine::n64 {

/** Fixed-capacity aggregation window used only by opt-in CPU profiling builds. */
class Nintendo64CpuProfileWindow final {
public:
    static constexpr std::uint32_t FramesPerReport = 60u;
    static constexpr std::size_t MaxStages = 24u;
    static constexpr std::size_t MaxComponents = 64u;

    struct StageRow {
        std::uint32_t StageId = 0u;
        std::uint64_t TotalMicroseconds = 0u;
        std::uint64_t MaxMicroseconds = 0u;
        std::uint32_t Calls = 0u;
    };

    struct ComponentRow {
        std::uintptr_t Identity = 0u;
        std::uint32_t Index = 0u;
        std::uint32_t TypeHash = 0u;
        std::uint32_t OwnerSceneEntityId = 0u;
        char TypeName[48]{};
        std::uint64_t TotalMicroseconds = 0u;
        std::uint64_t MaxMicroseconds = 0u;
        std::uint32_t Calls = 0u;
    };

    struct ReportView {
        std::uint64_t FirstSequence = 0u;
        std::uint64_t LastSequence = 0u;
        std::uint32_t FrameCount = 0u;
        std::uint32_t Dropped = 0u;
        std::uint32_t Resets = 0u;
        std::uint32_t ClockFaults = 0u;
        std::size_t StageCount = 0u;
        std::size_t ComponentCount = 0u;
        const StageRow* Stages = nullptr;
        const ComponentRow* Components = nullptr;
    };

    bool BeginFrame(std::uint64_t sequence);
    void AddStage(std::uint32_t stageId, std::uint64_t elapsedMicroseconds);
    void AddComponent(std::uintptr_t identity, std::uint32_t index, std::uint32_t typeHash,
                      std::uint32_t ownerSceneEntityId, const char* typeName,
                      std::uint64_t elapsedMicroseconds);
    bool EndFrame(std::uint64_t sequence);
    void ClockFault();
    bool IsReportReady() const { return ReportReady; }
    ReportView GetReport() const;
    void ClearReport();

private:
    void ResetWindow();
    StageRow Stages[MaxStages]{};
    ComponentRow Components[MaxComponents]{};
    std::size_t StageCount = 0u;
    std::size_t ComponentCount = 0u;
    std::uint64_t FirstSequence = 0u;
    std::uint64_t CurrentSequence = 0u;
    std::uint64_t LastSequence = 0u;
    std::uint32_t FrameCount = 0u;
    std::uint32_t Dropped = 0u;
    std::uint32_t Resets = 0u;
    std::uint32_t ClockFaults = 0u;
    bool FrameActive = false;
    bool ReportReady = false;
};

} // namespace helengine::n64
