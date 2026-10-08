#include "src/platform/n64/Nintendo64CpuProfileWindow.hpp"
#include "src/platform/n64/Nintendo64CpuProfileTypeName.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>

using helengine::n64::Nintendo64CpuProfileWindow;
using helengine::n64::DecodeItaniumTypeName;
using helengine::n64::StableTypeNameHash;

int main() {
    char decodedType[48]{};
    assert(DecodeItaniumTypeName("21CubeTestSpinComponent", decodedType));
    assert(std::strcmp(decodedType, "CubeTestSpinComponent") == 0);
    assert(DecodeItaniumTypeName("N4demo21CubeTestSpinComponentE", decodedType));
    assert(std::strcmp(decodedType, "CubeTestSpinComponent") == 0);
    assert(!DecodeItaniumTypeName("IUpdateable", decodedType));
    assert(!DecodeItaniumTypeName("N4demo99CubeTestSpinComponentE", decodedType));
    assert(StableTypeNameHash("SpinComponent") == 0x4893ea74u);

    Nintendo64CpuProfileWindow window;
    for (std::uint64_t sequence = 1; sequence <= Nintendo64CpuProfileWindow::FramesPerReport; ++sequence) {
        assert(window.BeginFrame(sequence));
        window.AddStage(3u, sequence == 1 ? 100u : 200u);
        assert(window.EndFrame(sequence));
    }

    assert(window.IsReportReady());
    Nintendo64CpuProfileWindow::ReportView report = window.GetReport();
    assert(report.FrameCount == 60u);
    assert(report.FirstSequence == 1u);
    assert(report.LastSequence == 60u);
    assert(report.StageCount == 1u);
    assert(report.Stages[0].StageId == 3u);
    assert(report.Stages[0].TotalMicroseconds == 11900u);
    assert(report.Stages[0].MaxMicroseconds == 200u);
    assert(report.Stages[0].Calls == 60u);

    window.ClearReport();
    assert(!window.IsReportReady());
    assert(window.GetReport().StageCount == 0u);
    assert(window.BeginFrame(61u));
    window.AddComponent(0x1000u, 2u, 77u, 9u, "SpinComponent", 40u);
    window.AddComponent(0x2000u, 2u, 77u, 9u, "SpinComponent", 80u);
    assert(window.EndFrame(61u));
    report = window.GetReport();
    assert(report.ComponentCount == 2u);
    assert(report.Components[0].Identity != report.Components[1].Identity);
    assert(report.Components[0].TypeHash == 77u);
    assert(std::strcmp(report.Components[0].TypeName, "SpinComponent") == 0);
    assert(report.Components[0].TotalMicroseconds == 40u);
    assert(report.Components[1].TotalMicroseconds == 80u);

    window.ClockFault();
    report = window.GetReport();
    assert(report.ClockFaults == 1u);
    assert(report.ComponentCount == 0u);
    return 0;
}
