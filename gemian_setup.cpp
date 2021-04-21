/*
 * Copyright (c) 2021, Adam Boardman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/klog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <memory>
#include <cstring>
#include <vector>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <bootloader_message/bootloader_message.h>
#include <cutils/android_reboot.h>
#include <cutils/properties.h> /* for property_list */
#include <health2/Health.h>
#include <private/android_filesystem_config.h> /* for AID_SYSTEM */
#include <private/android_logger.h>            /* private pmsg functions */
#include <selinux/android.h>
#include <selinux/label.h>
#include <selinux/selinux.h>
#include <ziparchive/zip_archive.h>

#include "adb_install.h"
#include "common.h"
#include "device.h"
#include "fuse_sdcard_provider.h"
#include "fuse_sideload.h"
#include "install.h"
#include "minadbd/minadbd.h"
#include "minui/minui.h"
#include "otautil/DirUtil.h"
#include "otautil/error_code.h"
#include "roots.h"
#include "rotate_logs.h"
#include "screen_ui.h"
#include "stub_ui.h"
#include "para_variables.h"
#include "gemian_install_config.h"
#include "ui.h"

static constexpr const char* DEFAULT_LOCALE = "en-US";

struct selabel_handle* sehandle;

ParaVariables paraVariables;
RecoveryUI *ui;
extern const char* keyboard_layout_codes[];
extern const char* keyboard_layout_items[];
extern const char** time_zone_items[];

/*
 * The recovery tool communicates with the main system through /cache files.
 *   /cache/recovery/command - INPUT - command line for tool, one arg per line
 *   /cache/recovery/log - OUTPUT - combined log file from recovery run(s)
 *
 * The arguments which may be supplied in the recovery.command file:
 *   --update_package=path - verify install an OTA package file
 *   --wipe_data - erase user data (and cache), then reboot
 *   --prompt_and_wipe_data - prompt the user that data is corrupt,
 *       with their consent erase user data (and cache), then reboot
 *   --wipe_cache - wipe cache (but not user data), then reboot
 *   --set_encrypted_filesystem=on|off - enables / diasables encrypted fs
 *   --just_exit - do nothing; exit and reboot
 *
 * After completing, we remove /cache/recovery/command and reboot.
 * Arguments may also be supplied in the bootloader control block (BCB).
 * These important scenarios must be safely restartable at any point:
 *
 * FACTORY RESET
 * 1. user selects "factory reset"
 * 2. main system writes "--wipe_data" to /cache/recovery/command
 * 3. main system reboots into recovery
 * 4. get_args() writes BCB with "boot-recovery" and "--wipe_data"
 *    -- after this, rebooting will restart the erase --
 * 5. erase_volume() reformats /data
 * 6. erase_volume() reformats /cache
 * 7. finish_recovery() erases BCB
 *    -- after this, rebooting will restart the main system --
 * 8. main() calls reboot() to boot main system
 *
 * OTA INSTALL
 * 1. main system downloads OTA package to /cache/some-filename.zip
 * 2. main system writes "--update_package=/cache/some-filename.zip"
 * 3. main system reboots into recovery
 * 4. get_args() writes BCB with "boot-recovery" and "--update_package=..."
 *    -- after this, rebooting will attempt to reinstall the update --
 * 5. install_package() attempts to install the update
 *    NOTE: the package install must itself be restartable from any point
 * 6. finish_recovery() erases BCB
 *    -- after this, rebooting will (try to) restart the main system --
 * 7. ** if install failed **
 *    7a. prompt_and_wait() shows an error icon and waits for the user
 *    7b. the user reboots (pulling the battery, etc) into the main system
 */

// Open a given path, mounting partitions as necessary.
FILE* fopen_path(const char* path, const char* mode) {
  /*if (ensure_path_mounted(path) != 0) {
    LOG(ERROR) << "Can't mount " << path;
    return nullptr;
  }*/

  // When writing, try to create the containing directory, if necessary. Use generous permissions,
  // the system (init.rc) will reset them.
  if (strchr("wa", mode[0])) {
    mkdir_recursively(path, 0777, true, sehandle);
  }
  return fopen(path, mode);
}

// close a file, log an error if the error indicator is set
static void check_and_fclose(FILE *fp, const char *name) {
    fflush(fp);
    if (fsync(fileno(fp)) == -1) {
        PLOG(ERROR) << "Failed to fsync " << name;
    }
    if (ferror(fp)) {
        PLOG(ERROR) << "Error in " << name;
    }
    fclose(fp);
}

bool reboot(const std::string&) {
	return false;
}

static void redirect_stdio(const char* filename) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        PLOG(ERROR) << "pipe failed";

        // Fall back to traditional logging mode without timestamps.
        // If these fail, there's not really anywhere to complain...
        freopen(filename, "a", stdout); setbuf(stdout, NULL);
        freopen(filename, "a", stderr); setbuf(stderr, NULL);

        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        PLOG(ERROR) << "fork failed";

        // Fall back to traditional logging mode without timestamps.
        // If these fail, there's not really anywhere to complain...
        freopen(filename, "a", stdout); setbuf(stdout, NULL);
        freopen(filename, "a", stderr); setbuf(stderr, NULL);

        return;
    }

    if (pid == 0) {
        /// Close the unused write end.
        close(pipefd[1]);

        auto start = std::chrono::steady_clock::now();

        // Child logger to actually write to the log file.
        FILE* log_fp = fopen(filename, "ae");
        if (log_fp == nullptr) {
            PLOG(ERROR) << "fopen \"" << filename << "\" failed";
            close(pipefd[0]);
            _exit(EXIT_FAILURE);
        }

        FILE* pipe_fp = fdopen(pipefd[0], "r");
        if (pipe_fp == nullptr) {
            PLOG(ERROR) << "fdopen failed";
            check_and_fclose(log_fp, filename);
            close(pipefd[0]);
            _exit(EXIT_FAILURE);
        }

        char* line = nullptr;
        size_t len = 0;
        while (getline(&line, &len, pipe_fp) != -1) {
            auto now = std::chrono::steady_clock::now();
            double duration = std::chrono::duration_cast<std::chrono::duration<double>>(
                    now - start).count();
            if (line[0] == '\n') {
                fprintf(log_fp, "[%12.6lf]\n", duration);
            } else {
                fprintf(log_fp, "[%12.6lf] %s", duration, line);
            }
            fflush(log_fp);
        }

        PLOG(ERROR) << "getline failed";

        free(line);
        check_and_fclose(log_fp, filename);
        close(pipefd[0]);
        _exit(EXIT_FAILURE);
    } else {
        // Redirect stdout/stderr to the logger process.
        // Close the unused read end.
        close(pipefd[0]);

        setbuf(stdout, nullptr);
        setbuf(stderr, nullptr);

        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            PLOG(ERROR) << "dup2 stdout failed";
        }
        if (dup2(pipefd[1], STDERR_FILENO) == -1) {
            PLOG(ERROR) << "dup2 stderr failed";
        }

        close(pipefd[1]);
    }
}

// Display a menu with the specified 'headers' and 'items'. Device specific HandleMenuKey() may
// return a positive number beyond the given range. Caller sets 'menu_only' to true to ensure only
// a menu item gets selected. 'initial_selection' controls the initial cursor location. Returns the
// (non-negative) chosen item number, or -1 if timed out waiting for input.
int get_menu_selection(bool menu_is_main, menu_type_t menu_type, const char* const* headers,
                       const MenuItemVector& menu_items, bool menu_only, int initial_selection,
                       Device* device, bool refreshable = false) {
  // Throw away keys pressed previously, so user doesn't accidentally trigger menu items.
  ui->FlushKeys();

  ui->StartMenu(menu_is_main, menu_type, headers, menu_items, initial_selection);

  int selected = initial_selection;
  int chosen_item = -1;
  while (chosen_item < 0) {
    RecoveryUI::InputEvent evt = ui->WaitInputEvent();
    if (evt.type() == RecoveryUI::EVENT_TYPE_NONE) {  // WaitKey() timed out.
      if (ui->WasTextEverVisible()) {
        continue;
      } else {
        ui->Print("Timed out waiting for key input; rebooting.");
        ui->EndMenu();
        return -1;
      }
    }

    int action = Device::kNoAction;
    if (evt.type() == RecoveryUI::EVENT_TYPE_TOUCH) {
      int touch_sel = ui->SelectMenu(evt.pos());
      if (touch_sel < 0) {
        action = touch_sel;
      } else {
        action = Device::kInvokeItem;
        selected = touch_sel;
      }
    } else {
      bool visible = ui->IsTextVisible();
      action = device->HandleMenuKey(evt.key(), visible);
    }

    if (action < 0) {
      switch (action) {
        case Device::kHighlightUp:
          selected = ui->SelectMenu(--selected);
          break;
        case Device::kHighlightDown:
          selected = ui->SelectMenu(++selected);
          break;
        case Device::kScrollUp:
          selected = ui->ScrollMenu(-1);
          break;
        case Device::kScrollDown:
          selected = ui->ScrollMenu(1);
          break;
        case Device::kInvokeItem:
          chosen_item = selected;
          if (chosen_item < 0) {
            chosen_item = Device::kGoBack;
          }
          break;
        case Device::kNoAction:
          break;
        case Device::kGoBack:
          chosen_item = Device::kGoBack;
          break;
        case Device::kRefresh:
          if (refreshable) {
            chosen_item = Device::kRefresh;
          }
          break;
      }
    } else if (!menu_only) {
      chosen_item = action;
    }
    if (chosen_item == Device::kGoBack ||
        chosen_item == Device::kRefresh) {
      break;
    }
  }

  ui->EndMenu();
  return chosen_item;
}

static void keyboard_layout(Device *device) {
    ui->Print("Keyboard layout: %s\n", paraVariables["keyboard_layout"].c_str());

    const char* headers[] = { "Select internal hardware keyboard layout", nullptr };

    int current_index = index_for_keyboard_layout(paraVariables["keyboard_layout"]);

    MenuItemVector items;
    for (int i=0; keyboard_layout_items[i] != nullptr; i++) {
      items.push_back(MenuItem(keyboard_layout_items[i]));
    }

    int chosen_item = get_menu_selection(false, MT_LIST, headers, items, true, current_index, device);
    if (chosen_item > 0) {
        paraVariables["keyboard_layout"] = keyboard_layout_codes[chosen_item];
        ui->Print("Keyboard layout saved: %s\n", keyboard_layout_codes[chosen_item]);
    }
}

static void time_zone_city_region(Device *device, int area_index) {
  const char* headers[] = { "Select city or region", nullptr };

  int current_area_index = index_for_tz_area(paraVariables["time_zone"]);
  int current_index = (area_index == current_area_index) ? index_for_tz_city_region(paraVariables["time_zone"]) : 0;
  MenuItemVector items;
  items.push_back(MenuItem("Back"));
  for (int i=1; time_zone_items[area_index][i] != nullptr; i++) {
    items.push_back(MenuItem(time_zone_items[area_index][i]));
  }

  int chosen_item = get_menu_selection(false, MT_LIST, headers, items, true, current_index, device);
  if (chosen_item > 0) {
    paraVariables["time_zone"] = std::string(time_zone_items[area_index][0]) + "/" + time_zone_items[area_index][chosen_item];
    ui->Print("TimeZone selected: %s\n", paraVariables["time_zone"].c_str());
  }
}

static void time_zone(Device *device) {
  ui->Print("TimeZone: %s\n", paraVariables["time_zone"].c_str());

  const char* headers[] = { "Select geographic area", nullptr };

  int current_index = index_for_tz_area(paraVariables["time_zone"]);
  MenuItemVector items;
  for (int i=0; i <= time_zone_areas_count; i++) {
    items.push_back(MenuItem(time_zone_items[i][0]));
  }

  int chosen_item = get_menu_selection(false, MT_LIST, headers, items, true, current_index, device);
  if (chosen_item > 0) {
    time_zone_city_region(device,chosen_item);
  }
}

static void setup_menu(Device* device) {
  const char* headers[] = { "Gemian Setup", nullptr };
  int chosen_item = 0;
  do {
    auto keyboardMenu = std::string(" Set Keyboard: ") + name_for_keyboard_layout(paraVariables["keyboard_layout"]);
    auto timeZoneMenu = " Set Timezone: " + paraVariables["time_zone"];
    const MenuItemVector items = {
      MenuItem(keyboardMenu),
      MenuItem(timeZoneMenu),
      MenuItem(" Continue")
    };
    chosen_item = get_menu_selection(false, MT_LIST, headers, items, true, chosen_item, device);

    switch (chosen_item) {
      case 0:
        keyboard_layout(device);
        break;
      case 1:
        time_zone(device);
        break;
    }

  } while (chosen_item < 2);

}

int main() {
  redirect_stdio("/tmp/gemian-install-config.log");

  Device* device = make_device();
  if (device == nullptr) {
    printf("Failed make device\n");
  }
  ui = device->GetUI();

  if (!ui->Init(DEFAULT_LOCALE)) {
    printf("Failed to initialize UI, exiting\n");
    return 1;
  }

  ui->SetSystemUpdateText(false);

  ui->SetBackground(RecoveryUI::NONE);

  sehandle = selinux_android_file_context_handle();
  selinux_android_set_sehandle(sehandle);
  if (!sehandle) {
    ui->Print("Warning: No file_contexts\n");
  }

  device->StartRecovery();

  std::string filename = "/dev/block/platform/bootdevice/by-name/para";

  std::ifstream basicIfstream(filename, std::ios::binary);
  if (!basicIfstream.is_open()) {
    ui->Print("Failed to open file\n");
  }

  if (paraVariables.ReadFromStream(basicIfstream) != ParaVarErrorNone) {
    ui->Print("Failed to read file\n");
  }
  basicIfstream.close();

  if (paraVariables["keyboard_layout"].length() == 0) {
    paraVariables["keyboard_layout"] = "us";
  }
  if (paraVariables["time_zone"].length() == 0) {
    paraVariables["time_zone"] = "Etc/UTC";
  }
  std::string keyboardAtLoad(paraVariables["keyboard_layout"]);
  std::string timeZoneAtLoad(paraVariables["time_zone"]);

  ui->SetProgressType(RecoveryUI::EMPTY);

  ui->ShowText(true);

  setup_menu(device);

  if ((keyboardAtLoad.compare(paraVariables["keyboard_layout"]) != 0)
      || (timeZoneAtLoad.compare(paraVariables["time_zone"]) != 0)) {
    ui->Print("Layout selected\n");
    std::ofstream basicOfstream(filename, std::ios::binary);
    if (basicOfstream.is_open()) {
      ui->Print("Writing to file\n");
      paraVariables.WriteToStream(basicOfstream);
    }
    basicOfstream.close();
  }
  return 0;
}
