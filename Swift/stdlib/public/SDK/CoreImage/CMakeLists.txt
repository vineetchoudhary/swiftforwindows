add_swift_library(swiftCoreImage IS_SDK_OVERLAY
  CoreImage.swift

  TARGET_SDKS OSX IOS IOS_SIMULATOR TVOS TVOS_SIMULATOR
  SWIFT_MODULE_DEPENDS Foundation ObjectiveC
  FRAMEWORK_DEPENDS_OSX QuartzCore
  FRAMEWORK_DEPENDS_IOS_TVOS CoreImage)

