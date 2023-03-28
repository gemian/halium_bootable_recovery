#define _DEFAULT_SOURCE

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <android-base/file.h>
#include <android-base/stringprintf.h>

#include <sys/select.h>

#include "minui/minui.h"

#define IMAGES_MAX    30
#define MARGIN 20

GRSurface *logo = nullptr;

static struct option options[] = {
		{"animate",     required_argument, 0, 'a'},
		{"progressbar", required_argument, 0, 'p'},
		{"stopafter",   required_argument, 0, 's'},
		{"text",        required_argument, 0, 't'},
		{"font",        required_argument, 0, 'f'},
		{"help",        no_argument,       0, 'h'},
		{0, 0,                             0, 0},
};

bool string_match(const char* string, const char* match) {
    return(strncmp(string, match, strlen(match)) == 0);
}

bool string_match_keep(const char* string, const char* match, char *keep) {
    bool to_keep = string_match(string, match);
    if (to_keep) {
        strcpy(keep,&string[strlen(match)]);
    }
    return to_keep;
}

int
osUpdateScreenInit() {
    GRRotation orientation = GRRotation::LEFT;
    char path[281];
    char line[200];
    char device[10];
    char keyboard[10];

    memset(line,0,sizeof(line));
    FILE *file = fopen("/default.prop", "r");
    int c = 0;
    while (file != NULL && (c = fgetc(file)) != EOF) {
        if (c == '\n') {
            if (string_match_keep(line, "ro.product.system.device=", device)) {
                fclose(file);
                file = NULL;
            }
            memset(line, 0, sizeof(line));
        } else {
            line[strlen(line)] = c;
        }
    }
    if (file) fclose(file);
    file = NULL;

    printf("device: %s\n", device);

    DIR *dir;
    if ((dir = opendir("/sys/class/extcon"))) {

        struct dirent *de;
        while ((de = readdir(dir))) {
            std::string name_file = android::base::StringPrintf("/sys/class/extcon/%s/name", de->d_name);
            std::string name;
            if (!android::base::ReadFileToString(name_file, &name)) continue;
            if (strncmp(name.c_str(), "hall", 4)) continue;
            snprintf(path, sizeof(path) - 1, "/sys/class/extcon/%s/state", de->d_name);
            path[sizeof(path) - 1] = '\0';

            memset(line, 0, sizeof(line));
            FILE *file = fopen(path, "r");
            int c = 0;
            while (file != NULL && (c = fgetc(file)) != EOF) {
                if (c == '\n') {
                    if (string_match_keep(line, "MECHANICAL=", keyboard)) {
                        fclose(file);
                        file = NULL;
                    }
                    memset(line, 0, sizeof(line));
                } else {
                    line[strlen(line)] = c;
                }
            }
            if (file) fclose(file);
            file = NULL;

            if (string_match(device, "Astro")) {
                if (strcmp(keyboard, "1") == 0) {
                    orientation = GRRotation::RIGHT;
                } else {
                    orientation = GRRotation::NONE;
                }
            }
            if (string_match(device, "Cosmo")) {
                orientation = GRRotation::LEFT;
            }

        }
        closedir(dir);

        printf("keyboard: %s\n", keyboard);
    } else {
        perror("Can't open directory /sys/class/extcon - skipped auto-rotation");
    }

	if (gr_init(orientation)) {
		printf("Failed gr_init!\n");
		return -1;
	}

	/* Clear the screen */
	gr_color(0, 0, 0, 255);
	gr_clear();

	return 0;
}

/* ------------------------------------------------------------------------ */

int
loadLogo(std::string &filename) {
	int ret;

	if (logo)
		res_free_surface(logo);

	if ((ret = res_create_display_surface(filename.c_str(), &logo)) < 0) {
		printf("Error while trying to load %s, retval: %i.\n",
			   filename.c_str(), ret);
		return -1;
	}

	return 0;
}

/* ------------------------------------------------------------------------ */

int
showLogo(void) {
	int fbw = gr_fb_width();
	int fbh = gr_fb_height();

	/* draw logo to middle of the screen */
	if (logo) {
		int logow = gr_get_width(logo);
		int logoh = gr_get_height(logo);
		int dx = (fbw - logow) / 2;
		int dy = (fbh - logoh) / 2;

		gr_blit(logo, 0, 0, logow, logoh, dx, dy);
		gr_flip();
	} else {
		printf("No logo loaded\n");
		return -1;
	}

	return 0;
}

/* ------------------------------------------------------------------------ */

void
osUpdateScreenShowProgress(int percentage) {
	int fbw, fbh, splitpoint, x1, x2, y1, y2;

	fbw = gr_fb_width();
	fbh = gr_fb_height();

	splitpoint = (fbw - 2 * MARGIN) * percentage / 100;

	assert(splitpoint >= 0);
	assert(splitpoint <= fbw);

	x1 = MARGIN;
	y1 = fbh / 2 + MARGIN;
	x2 = MARGIN + splitpoint;
	y2 = fbh / 2 + 20;

	/* white color for the beginning of the progressbar */
	gr_color(255, 255, 255, 255);

	gr_fill(x1, y1, x2, y2);

	/* Grey color for the end part of the progressbar */
	gr_color(84, 84, 84, 255);

	x1 = MARGIN + splitpoint;
	x2 = fbw - MARGIN;

	gr_fill(x1, y1, x2, y2);

	/* draw logo on the top of the progress bar if it is loaded */
	if (logo) {
		int logow, logoh, dx, dy;

		logow = gr_get_width(logo);
		logoh = gr_get_height(logo);
		dx = (fbw - logow) / 2;
		dy = (fbh / 2 - logoh - 2 * MARGIN);
#ifdef DEBUG
		printf("width: %i, height: %i, row_bytes: %i, pixel_bytes: "
			   "%i\n",
			   logo->width, logo->height, logo->row_bytes,
			   logo->pixel_bytes);
#endif /* DEBUG */

		gr_blit(logo, 0, 0, logow, logoh, dx, dy);
	}

	/* And finally draw everything */
	gr_flip();
}

/* ------------------------------------------------------------------------ */

void
osUpdateScreenExit(void) {
	if (logo)
		res_free_surface(logo);

	gr_exit();
}

/* ------------------------------------------------------------------------ */

static void
short_help(void) {
	printf("  os-update-minui [OPTIONS] [IMAGE(s)]\n");
}

/* ------------------------------------------------------------------------ */

static void
print_help(void) {
	printf("  yamui - tool to display progress bar, logo, or small animation on UI\n");
	printf("  Usage:\n");
	short_help();
	printf("    IMAGE(s)   - png picture file names in /res/images without .png extension\n");
	printf("                 NOTE: currently maximum of %d pictures supported\n",
		   IMAGES_MAX);
	printf("\n  OPTIONS:\n");
	printf("  --animate=PERIOD, -a PERIOD\n");
	printf("         Show IMAGEs (at least 2) in rotation over PERIOD ms\n");
	printf("  --progressbar=TIME, -p TIME\n");
	printf("         Show a progess bar over TIME milliseconds\n");
	printf("  --stopafter=TIME, -s TIME\n");
	printf("         Stop showing the IMAGE(s) after TIME milliseconds\n");
	printf("  --text=STRING, -t STRING\n");
	printf("         Show STRING on the screen\n");
//	printf("  --font=N, -f N\n");
//	printf("         Multiply the text font size by N (default 1)\n");
	printf("  --help, -h\n");
	printf("         Print this help\n");
}

/* ------------------------------------------------------------------------ */

/* Add text to both sides of the "flip" */
static void
add_text(char *text) {
	int i = 0;
	int x = 20;
	int y = 20;
	if (!text)
		return;

	int text_width = gr_measure(gr_sys_font(), text);
	if (text_width < gr_fb_width()) {
		x = (gr_fb_width() - text_width) / 2;
	}

	y = gr_fb_height() * 0.75;

	for (i = 0; i < 2; i++) {
		gr_color(255, 255, 255, 255);
		gr_text(gr_sys_font(), x, y, text, 1);
		gr_flip();
	}
}

/* ------------------------------------------------------------------------ */

int
main(int argc, char *argv[]) {
	int c, option_index;
	unsigned long int animate_ms = 0;
	unsigned long long int stop_ms = 0;
	unsigned long long int progress_ms = 0;
//	unsigned long int scale_font_by = 1;
	char *text = nullptr;
	char *images[IMAGES_MAX];
	int image_count = 0;
	int ret = 0;
	int i = 0;

	while (true) {
		c = getopt_long(argc, argv, "a:p:s:t:f:h", options,
						&option_index);
		if (c == -1)
			break;

		switch (c) {
			case 'a':
				printf("got animate %s ms\n", optarg);
				animate_ms = strtoul(optarg, (char **) nullptr, 10);
				break;
			case 'p':
				printf("got progressbar %s ms\n", optarg);
				progress_ms = strtoull(optarg, (char **) nullptr, 10);
				break;
			case 's':
				printf("got stop at %s ms\n", optarg);
				stop_ms = strtoull(optarg, (char **) nullptr, 10);
				break;
			case 't':
				printf("got text \"%s\" to display\n", optarg);
				text = optarg;
				break;
//		case 'f':
//			printf("got font multiplier \"%s\"\n", optarg);
//			scale_font_by = strtoul(optarg, (char **)nullptr, 10);
//			break;
			case 'h':
				print_help();
				goto out;
				break;
			default:
				printf("getopt returned character code 0%o\n", c);
				short_help();
				goto out;
				break;
		}
	}

	while (optind < argc && image_count < IMAGES_MAX)
		images[image_count++] = argv[optind++];

	if (osUpdateScreenInit())
		return -1;

	/* In case there is text to add, add it to both sides of the "flip" */
	add_text(text);

	if (image_count == 1 && !progress_ms) {
		std::string imagePath("/res/images/");
		if (images[0][0] == '/') imagePath.clear();
		imagePath.append(images[0]);
		ret = loadLogo(imagePath);
		if (ret) {
			printf("Image \"%s\" not found in /res/images/\n",
				   images[0]);
			goto cleanup;
		}

		showLogo();
		if (stop_ms)
			usleep(stop_ms * 1000);
		else
			select(0, NULL, NULL, NULL, NULL);

		goto cleanup;
	}

	if (image_count <= 1 && progress_ms) {
		if (image_count == 1) {
			std::string imagePath("/res/images/");
			if (images[0][0] == '/') imagePath.clear();
			imagePath.append(images[0]);
			loadLogo(imagePath);
		}
		i = 0;
		while (i <= 100) {
			osUpdateScreenShowProgress(i);
			usleep(1000 * progress_ms / 100);
			i++;
		}

		goto cleanup;
	}

	if (image_count > 1 && progress_ms) {
		printf("Can only show one image with progressbar\n");
		goto cleanup;
	}

	if (animate_ms) {
		bool never_stop;
		long int time_left = stop_ms;
		int period = animate_ms / image_count;

		if (image_count < 2) {
			printf("Animating requires at least 2 images\n");
			goto cleanup;
		}

		if (stop_ms)
			never_stop = false;
		else
			never_stop = true;

		i = 0;
		while (never_stop || time_left > 0) {
			std::string imagePath("/res/images/");
			if (images[i][0] == '/') imagePath.clear();
			imagePath.append(images[i]);
			ret = loadLogo(imagePath);
			if (ret) {
				printf("\"%s\" not found in /res/images/\n",
					   images[i]);
				goto cleanup;
			}

			showLogo();
			usleep(1000 * period);
			time_left -= period;
			i++;
			i = i % image_count;
		}

		goto cleanup;
	}

	if (text) {
		if (stop_ms)
			usleep(1000 * stop_ms);
		else
			select(0, nullptr, nullptr, nullptr, nullptr);
		goto cleanup;
	}

	cleanup:
	osUpdateScreenExit();
	out:
	return ret;
}
