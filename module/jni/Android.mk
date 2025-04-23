LOCAL_PATH := $(call my-dir)

DOBBY_DEBUG := true
NEAR_BRANCH := true
FULL_FLOATING_POINT_REGISTER_PACK := false
PLUGIN_SYMBOL_RESOLVER := true

include $(CLEAR_VARS)

LOCAL_MODULE := dobby
LOCAL_STL := c++_shared

LOCAL_CPPFLAGS := -std=c++17 -Wall -Wextra -fvisibility=hidden -fvisibility-inlines-hidden -fno-rtti -fno-exceptions
LOCAL_CFLAGS := -std=c17 -Wall -Wextra -fvisibility=hidden

DOBBY_BUILD_VERSION := Dobby-Android-CMake-Translated
LOCAL_CFLAGS += -D__DOBBY_BUILD_VERSION__=\"$(DOBBY_BUILD_VERSION)\"

ifeq ($(DOBBY_DEBUG),true)
    LOCAL_CFLAGS += -DDOBBY_DEBUG
else
    LOCAL_CFLAGS += -DDOBBY_LOGGING_DISABLE
endif

ifeq ($(FULL_FLOATING_POINT_REGISTER_PACK),true)
    LOCAL_CFLAGS += -DFULL_FLOATING_POINT_REGISTER_PACK
endif

ifeq ($(NEAR_BRANCH),true)
    ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
        LOCAL_CFLAGS += -DNEAR_BRANCH_ENABLED
    endif
endif

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/Dobby \
    $(LOCAL_PATH)/Dobby/include \
    $(LOCAL_PATH)/Dobby/source \
    $(LOCAL_PATH)/Dobby/source/core \
    $(LOCAL_PATH)/Dobby/source/core/arch \
    $(LOCAL_PATH)/Dobby/source/dobby \
    $(LOCAL_PATH)/Dobby/external \
    $(LOCAL_PATH)/Dobby/external/logging \
    $(LOCAL_PATH)/Dobby/builtin-plugin \
    $(LOCAL_PATH)/Dobby/source/Backend/UserMode

LOCAL_SRC_FILES := \
    Dobby/source/core/arch/CpuFeature.cc \
    Dobby/source/core/arch/CpuRegister.cc \
    Dobby/source/core/assembler/assembler.cc \
    Dobby/source/core/assembler/assembler-arm.cc \
    Dobby/source/core/assembler/assembler-arm64.cc \
    Dobby/source/core/assembler/assembler-ia32.cc \
    Dobby/source/core/assembler/assembler-x64.cc \
    Dobby/source/core/codegen/codegen-arm.cc \
    Dobby/source/core/codegen/codegen-arm64.cc \
    Dobby/source/core/codegen/codegen-ia32.cc \
    Dobby/source/core/codegen/codegen-x64.cc \
    Dobby/source/MemoryAllocator/CodeBuffer/CodeBufferBase.cc \
    Dobby/source/MemoryAllocator/AssemblyCodeBuilder.cc \
    Dobby/source/MemoryAllocator/MemoryAllocator.cc \
    Dobby/source/InstructionRelocation/arm/InstructionRelocationARM.cc \
    Dobby/source/InstructionRelocation/arm64/InstructionRelocationARM64.cc \
    Dobby/source/InstructionRelocation/x86/InstructionRelocationX86.cc \
    Dobby/source/InstructionRelocation/x86/InstructionRelocationX86Shared.cc \
    Dobby/source/InstructionRelocation/x64/InstructionRelocationX64.cc \
    Dobby/source/InstructionRelocation/x86/x86_insn_decode/x86_insn_decode.c \
    Dobby/source/InterceptRouting/InterceptRouting.cpp \
    Dobby/source/TrampolineBridge/Trampoline/arm/trampoline_arm.cc \
    Dobby/source/TrampolineBridge/Trampoline/arm64/trampoline_arm64.cc \
    Dobby/source/TrampolineBridge/Trampoline/x86/trampoline_x86.cc \
    Dobby/source/TrampolineBridge/Trampoline/x64/trampoline_x64.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/arm/helper_arm.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/arm/closure_bridge_arm.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/arm/ClosureTrampolineARM.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/arm64/helper_arm64.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/arm64/closure_bridge_arm64.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/arm64/ClosureTrampolineARM64.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/x86/helper_x86.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/x86/closure_bridge_x86.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/x86/ClosureTrampolineX86.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/x64/helper_x64.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/x64/closure_bridge_x64.cc \
    Dobby/source/TrampolineBridge/ClosureTrampolineBridge/x64/ClosureTrampolineX64.cc \
    Dobby/source/InterceptRouting/Routing/InstructionInstrument/InstructionInstrument.cc \
    Dobby/source/InterceptRouting/Routing/InstructionInstrument/RoutingImpl.cc \
    Dobby/source/InterceptRouting/Routing/InstructionInstrument/instrument_routing_handler.cc \
    Dobby/source/InterceptRouting/Routing/FunctionInlineHook/FunctionInlineHook.cc \
    Dobby/source/InterceptRouting/Routing/FunctionInlineHook/RoutingImpl.cc \
    Dobby/source/InterceptRouting/RoutingPlugin/RoutingPlugin.cc \
    Dobby/source/dobby.cpp \
    Dobby/source/Interceptor.cpp \
    Dobby/source/InterceptEntry.cpp \
    Dobby/source/Backend/UserMode/PlatformUtil/Linux/ProcessRuntimeUtility.cc \
    Dobby/source/Backend/UserMode/UnifiedInterface/platform-posix.cc \
    Dobby/source/Backend/UserMode/ExecMemory/code-patch-tool-posix.cc \
    Dobby/source/Backend/UserMode/ExecMemory/clear-cache-tool-all.c \
    Dobby/external/logging/logging.cc \
    Dobby/external/logging/logging_kern.cc

ifeq ($(NEAR_BRANCH),true)
    ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
        LOCAL_SRC_FILES += \
            Dobby/source/InterceptRouting/RoutingPlugin/NearBranchTrampoline/near_trampoline_arm64.cc \
            Dobby/source/InterceptRouting/RoutingPlugin/NearBranchTrampoline/NearBranchTrampoline.cc \
            Dobby/source/MemoryAllocator/NearMemoryAllocator.cc
    endif
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/Dobby/builtin-plugin/SymbolResolver
LOCAL_SRC_FILES += \
    Dobby/builtin-plugin/SymbolResolver/elf/dobby_symbol_resolver.cc

LOCAL_LDLIBS := -llog -ldl

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := simulatetablet
LOCAL_STL := c++_shared

LOCAL_SRC_FILES := main.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/Dobby/include \

LOCAL_SHARED_LIBRARIES := dobby

LOCAL_LDLIBS := -llog -lstdc++

include $(BUILD_SHARED_LIBRARY)