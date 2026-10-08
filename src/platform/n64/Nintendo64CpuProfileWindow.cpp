#include "src/platform/n64/Nintendo64CpuProfileWindow.hpp"

#include <limits>

namespace helengine::n64 {

bool Nintendo64CpuProfileWindow::BeginFrame(std::uint64_t sequence) {
    if (ReportReady) {
        ++Dropped;
        return false;
    }
    if (FrameActive || (FrameCount != 0u && sequence != LastSequence + 1u)) {
        ++Resets;
        ResetWindow();
    }
    FrameActive = true;
    CurrentSequence = sequence;
    return true;
}

void Nintendo64CpuProfileWindow::AddStage(std::uint32_t stageId, std::uint64_t elapsedMicroseconds) {
    if (!FrameActive) return;
    for (std::size_t i = 0u; i < StageCount; ++i) {
        StageRow& row = Stages[i];
        if (row.StageId != stageId) continue;
        if (elapsedMicroseconds > std::numeric_limits<std::uint64_t>::max() - row.TotalMicroseconds) {
            ++Dropped;
            return;
        }
        row.TotalMicroseconds += elapsedMicroseconds;
        if (elapsedMicroseconds > row.MaxMicroseconds) row.MaxMicroseconds = elapsedMicroseconds;
        ++row.Calls;
        return;
    }
    if (StageCount >= MaxStages) {
        ++Dropped;
        return;
    }
    StageRow& row = Stages[StageCount++];
    row.StageId = stageId;
    row.TotalMicroseconds = elapsedMicroseconds;
    row.MaxMicroseconds = elapsedMicroseconds;
    row.Calls = 1u;
}

void Nintendo64CpuProfileWindow::AddComponent(std::uintptr_t identity, std::uint32_t index,
                                               std::uint32_t typeHash, std::uint32_t ownerSceneEntityId,
                                               const char* typeName, std::uint64_t elapsedMicroseconds) {
    if (!FrameActive) return;
    for (std::size_t i = 0u; i < ComponentCount; ++i) {
        ComponentRow& row = Components[i];
        if (row.Identity != identity) continue;
        if (elapsedMicroseconds > std::numeric_limits<std::uint64_t>::max() - row.TotalMicroseconds) {
            ++Dropped;
            return;
        }
        row.TotalMicroseconds += elapsedMicroseconds;
        if (elapsedMicroseconds > row.MaxMicroseconds) row.MaxMicroseconds = elapsedMicroseconds;
        ++row.Calls;
        return;
    }
    if (ComponentCount >= MaxComponents) {
        ++Dropped;
        return;
    }
    ComponentRow& row = Components[ComponentCount++];
    row.Identity = identity;
    row.Index = index;
    row.TypeHash = typeHash;
    row.OwnerSceneEntityId = ownerSceneEntityId;
    std::size_t length = 0u;
    while (typeName != nullptr && length + 1u < sizeof(row.TypeName) && typeName[length] != '\0') ++length;
    for (std::size_t i = 0u; i < length; ++i) row.TypeName[i] = typeName[i];
    row.TypeName[length] = '\0';
    row.TotalMicroseconds = elapsedMicroseconds;
    row.MaxMicroseconds = elapsedMicroseconds;
    row.Calls = 1u;
}

bool Nintendo64CpuProfileWindow::EndFrame(std::uint64_t sequence) {
    if (!FrameActive || sequence != CurrentSequence) {
        ++Resets;
        ResetWindow();
        return false;
    }
    if (FrameCount == 0u) FirstSequence = sequence;
    LastSequence = sequence;
    ++FrameCount;
    FrameActive = false;
    ReportReady = FrameCount >= FramesPerReport;
    return true;
}

void Nintendo64CpuProfileWindow::ClockFault() {
    ++ClockFaults;
    ++Resets;
    ResetWindow();
}

Nintendo64CpuProfileWindow::ReportView Nintendo64CpuProfileWindow::GetReport() const {
    ReportView report;
    report.FirstSequence = FirstSequence;
    report.LastSequence = LastSequence;
    report.FrameCount = FrameCount;
    report.Dropped = Dropped;
    report.Resets = Resets;
    report.ClockFaults = ClockFaults;
    report.StageCount = StageCount;
    report.ComponentCount = ComponentCount;
    report.Stages = Stages;
    report.Components = Components;
    return report;
}

void Nintendo64CpuProfileWindow::ClearReport() {
    ResetWindow();
}

void Nintendo64CpuProfileWindow::ResetWindow() {
    for (StageRow& row : Stages) row = StageRow{};
    for (ComponentRow& row : Components) row = ComponentRow{};
    StageCount = 0u;
    ComponentCount = 0u;
    FirstSequence = 0u;
    CurrentSequence = 0u;
    LastSequence = 0u;
    FrameCount = 0u;
    FrameActive = false;
    ReportReady = false;
}

} // namespace helengine::n64
