#include "platform/n64/Nintendo64CpuProfileSink.hpp"

#include "IUpdateable.hpp"
#include "platform/n64/Nintendo64CpuProfileTypeName.hpp"
#include <libdragon.h>
#include <typeinfo>

namespace helengine::n64 {
namespace {

const char* StageName(std::uint32_t id) {
    switch (id) {
        case 1u: return "early_input";
        case 2u: return "frame_counters";
        case 3u: return "object_manager_update";
        case 4u: return "audio";
        case 5u: return "physics";
        case 6u: return "late_input";
        case 7u: return "pointer_interaction";
        case 8u: return "frame_boundary";
        case 9u: return "render_manager_3d";
        case Nintendo64CpuProfileSink::HostUpdate: return "host_update";
        case Nintendo64CpuProfileSink::RenderManager2D: return "render_manager_2d";
        case Nintendo64CpuProfileSink::RdpqDetachWait: return "rdpq_detach_wait";
        default: return "unknown";
    }
}

} // namespace

std::uint64_t Nintendo64CpuProfileSink::ReadMicroseconds() {
    return get_ticks_us();
}

void Nintendo64CpuProfileSink::RecordStage(::RuntimeCpuProfileStage stage, std::uint64_t elapsedMicroseconds) {
    Window.AddStage(static_cast<std::uint32_t>(stage), elapsedMicroseconds);
}

void Nintendo64CpuProfileSink::RecordUpdateable(::IUpdateable* item, std::int32_t index,
                                                 std::uint32_t typeHash,
                                                 std::uint32_t ownerSceneEntityId,
                                                 std::uint64_t elapsedMicroseconds) {
    const std::uintptr_t identity = reinterpret_cast<std::uintptr_t>(item);
    const TypeRegistration* registration = FindTypeRegistration(identity);
    char decodedName[48]{};
    const bool decoded = item != nullptr && DecodeItaniumTypeName(typeid(*item).name(), decodedName);
    const char* typeName = decoded ? decodedName : FindTypeName(identity, typeHash);
    const std::uint32_t resolvedTypeHash = decoded ? StableTypeNameHash(decodedName)
        : registration == nullptr ? typeHash : registration->TypeHash;
    Window.AddComponent(identity, index < 0 ? 0u : static_cast<std::uint32_t>(index),
                        resolvedTypeHash, ownerSceneEntityId, typeName, elapsedMicroseconds);
}

void Nintendo64CpuProfileSink::RegisterType(::IUpdateable* item, std::uint32_t typeHash, HeCppString typeName) {
    if (item == nullptr) return;
    const std::uintptr_t identity = reinterpret_cast<std::uintptr_t>(item);
    TypeRegistration* registration = nullptr;
    for (std::size_t i = 0u; i < TypeRegistrationCount; ++i) {
        if (TypeRegistrations[i].Identity == identity) {
            registration = &TypeRegistrations[i];
            break;
        }
    }
    if (registration == nullptr && TypeRegistrationCount < MaxTypeRegistrations) {
        registration = &TypeRegistrations[TypeRegistrationCount++];
    }
    if (registration == nullptr) return;
    registration->Identity = identity;
    char decodedName[sizeof(registration->Name)]{};
    const bool decoded = DecodeItaniumTypeName(typeid(*item).name(), decodedName);
    const char* name = decoded ? decodedName : typeName.c_str();
    std::size_t length = 0u;
    while (name != nullptr && length + 1u < sizeof(registration->Name) && name[length] != '\0') ++length;
    for (std::size_t i = 0u; i < length; ++i) registration->Name[i] = name[i];
    registration->Name[length] = '\0';
    registration->TypeHash = decoded ? StableTypeNameHash(registration->Name) : typeHash;
}

void Nintendo64CpuProfileSink::ClockFault() {
    Window.ClockFault();
}

void Nintendo64CpuProfileSink::BeginFrame(std::uint64_t sequence) {
    Window.BeginFrame(sequence);
}

void Nintendo64CpuProfileSink::EndFrame(std::uint64_t sequence) {
    Window.EndFrame(sequence);
}

void Nintendo64CpuProfileSink::RecordNativeStage(std::uint32_t stageId, std::uint64_t elapsedMicroseconds) {
    Window.AddStage(stageId, elapsedMicroseconds);
}

const char* Nintendo64CpuProfileSink::FindTypeName(std::uintptr_t identity, std::uint32_t typeHash) const {
    (void)typeHash;
    const TypeRegistration* registration = FindTypeRegistration(identity);
    return registration == nullptr ? "unknown" : registration->Name;
}

const Nintendo64CpuProfileSink::TypeRegistration*
Nintendo64CpuProfileSink::FindTypeRegistration(std::uintptr_t identity) const {
    for (std::size_t i = 0u; i < TypeRegistrationCount; ++i) {
        const TypeRegistration& registration = TypeRegistrations[i];
        if (registration.Identity == identity) return &registration;
    }
    return nullptr;
}

void Nintendo64CpuProfileSink::EmitPendingReport() {
    if (!Window.IsReportReady()) return;
    const Nintendo64CpuProfileWindow::ReportView report = Window.GetReport();
    debugf("n64:cpu-profile frames=%lu first=%llu last=%llu clock=get_ticks_us dropped=%lu resets=%lu clock_faults=%lu\n",
           static_cast<unsigned long>(report.FrameCount), static_cast<unsigned long long>(report.FirstSequence),
           static_cast<unsigned long long>(report.LastSequence), static_cast<unsigned long>(report.Dropped),
           static_cast<unsigned long>(report.Resets), static_cast<unsigned long>(report.ClockFaults));
    for (std::size_t i = 0u; i < report.StageCount; ++i) {
        const Nintendo64CpuProfileWindow::StageRow& row = report.Stages[i];
        const std::uint64_t average = row.Calls == 0u ? 0u : row.TotalMicroseconds / row.Calls;
        debugf("n64:cpu-profile-stage id=%lu name=%s avg_us=%llu max_us=%llu calls=%lu\n",
               static_cast<unsigned long>(row.StageId), StageName(row.StageId),
               static_cast<unsigned long long>(average), static_cast<unsigned long long>(row.MaxMicroseconds),
               static_cast<unsigned long>(row.Calls));
    }
    for (std::size_t i = 0u; i < report.ComponentCount; ++i) {
        const Nintendo64CpuProfileWindow::ComponentRow& row = report.Components[i];
        const std::uint64_t average = row.Calls == 0u ? 0u : row.TotalMicroseconds / row.Calls;
        debugf("n64:cpu-profile-component index=%lu type=%s hash=%08lx owner=%lu identity=%p avg_us=%llu max_us=%llu calls=%lu\n",
               static_cast<unsigned long>(row.Index), row.TypeName, static_cast<unsigned long>(row.TypeHash),
               static_cast<unsigned long>(row.OwnerSceneEntityId), reinterpret_cast<const void*>(row.Identity),
               static_cast<unsigned long long>(average), static_cast<unsigned long long>(row.MaxMicroseconds),
               static_cast<unsigned long>(row.Calls));
    }
    Window.ClearReport();
}

} // namespace helengine::n64
