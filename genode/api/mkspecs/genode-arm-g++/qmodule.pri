host_build {
    QT_CPU_FEATURES.x86_64 = mmx sse sse2
} else {
    QT_CPU_FEATURES.arm = 
}
QT.global_private.enabled_features = alloca gui network reduce_exports release_tools sql testlib widgets xml
QT.global_private.disabled_features = sse2 alloca_h alloca_malloc_h android-style-assets avx2 private_tests dbus dbus-linked gc_binaries libudev posix_fallocate reduce_relocations stack-protector-strong system-zlib zstd
QT_COORD_TYPE = double
CONFIG -= precompile_header
CONFIG += cross_compile compile_examples largefile nostrip
QT_BUILD_PARTS += libs examples
QT_HOST_CFLAGS_DBUS += 
