//
// Copyright (c) 2026 Nightwind
// Original implementation: https://github.com/tealbathingsuit/OldABI/blob/main/OldABI.swift
//

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <limits.h>
#include <mach-o/dyld.h>

// This macro only exists in https://github.com/roothide/theos
#ifdef THEOS_PACKAGE_SCHEME_ROOTHIDE
#include <roothide.h>
#endif

#include "patchfind.h"
#include "subtype.h"
#include "utils.h"

static const char *const whitelist[] = {
	"/System/Library/CoreServices/SpringBoard.app/SpringBoard",
	"/Applications/",
	"/usr/sbin/mediaserverd",
	"/usr/libexec/backboardd",
	"/usr/libexec/nfcd"
};

extern void MSHookMemory(void *target, const void *data, size_t size);

static void patch_image(const uint32_t dyld_image_index) {
	static const uint8_t bytes[4] = { 0x00, 0x18, 0xC1, 0xDA };
	static const uint8_t mask[4] = { 0x00, 0xFC, 0xFF, 0xFF };

	const intptr_t base_addr = _dyld_get_image_vmaddr_slide(dyld_image_index);
	const struct mach_header_64 *const header = (const struct mach_header_64 *)_dyld_get_image_header(dyld_image_index);

	const struct segment_command_64 *cmd = NULL;

	uintptr_t addr = (uintptr_t)(header + 1);
	const uintptr_t end_addr = addr + header->sizeofcmds;

	const size_t byte_count = sizeof(bytes);

	const uint32_t patch_bytes = OSSwapBigToHostInt32(0xF047C1DA); // xpacd x16

	for (int ci = 0; ci < header->ncmds && addr <= end_addr; ci++) {
		cmd = (__typeof__(cmd))addr;

		addr = addr + cmd->cmdsize;

		if (cmd->cmd != LC_SEGMENT_64 || strcmp(cmd->segname, SEG_TEXT) != 0) {
			continue;
		}

		vm_address_t haystack_base = cmd->vmaddr + base_addr;
		vm_offset_t haystack_length = cmd->vmsize;

		while (haystack_base && haystack_length > 0) {
			char *found = patchfind_in_region(haystack_base, haystack_length, bytes, mask, byte_count);
			if (found == NULL) {
				break;
			}

			MSHookMemory(found, &patch_bytes, sizeof(patch_bytes));

			found += byte_count;
			haystack_length -= ((char *)found - (char *)haystack_base);
			haystack_base = (vm_address_t)found;
		}
	}
}

static void oneshot_fix_oldabi(void) {
	static const char *patch_list[] = {
		"/usr/lib/libobjc.A.dylib",
		"/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation",
		// "/usr/lib/swift/libswiftCore.dylib", (cannot reliably get this working without respring loops)
	};

	bool has_unpatched = false;
	for (unsigned patch_list_index = 0; patch_list_index < (sizeof(patch_list) / sizeof(char *)); patch_list_index++) {
		if (patch_list[patch_list_index] != NULL) {
			has_unpatched = true;
			break;
		}
	}

	if (!has_unpatched) {
		return;
	}

	const uint32_t image_count = _dyld_image_count();

	for (uint32_t i = 0; i < image_count; ++i) {
		const char *const image_name = _dyld_get_image_name(i);

		for (unsigned patch_list_index = 0; patch_list_index < (sizeof(patch_list) / sizeof(char *)); patch_list_index++) {
			const char *patch_image_name = patch_list[patch_list_index];
			if (patch_image_name != NULL && strcmp(image_name, patch_image_name) == 0) {
				patch_image(i);
				patch_list[patch_list_index] = NULL;
			}
		}
	}
}

static void add_image_callback(const struct mach_header *header, intptr_t vmaddr_slide) {
	if (header == NULL) {
		return;
	}

	if (header_looks_legacy(header) == true) {
		oneshot_fix_oldabi();
	}
}

__attribute__((constructor)) static void ctor(void) {
    char executable_path[PATH_MAX];
    uint32_t executable_path_size = sizeof(executable_path);

    if (_NSGetExecutablePath(executable_path, &executable_path_size) != 0) {
        return;
    }

	const size_t whitelist_length = sizeof(whitelist) / sizeof(whitelist[0]);

	bool is_whitelisted = false;

	for (size_t i = 0; i < whitelist_length; ++i) {
		if (string_has_prefix(executable_path, whitelist[i]) == true) {
			is_whitelisted = true;
			break;
		}
	}

#ifdef THEOS_PACKAGE_SCHEME_ROOTHIDE // This macro only exists in https://github.com/roothide/theos
	if (is_whitelisted == false && string_has_prefix(rootfs(executable_path), "/Applications/") == false) {
#else
	if (is_whitelisted == false && strstr(executable_path, "/procursus/Applications/") == NULL) {
#endif
		return;
	}

	_dyld_register_func_for_add_image(&add_image_callback);
}
