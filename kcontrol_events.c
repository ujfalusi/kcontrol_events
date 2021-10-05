// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A simple tool to monitor ALSA control events.
 */

#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char card[64] = "hw:0";
static char *control_name;
int num_events;

static int help(void)
{
	printf("Usage: kcontrol_events <options>\n");
	printf("\nAvailable options:\n");
	printf("  -h,--help      this help\n");
	printf("  -c,--card N    select the card, default 0\n");
	printf("  -D,--device N  select the device, default '%s'\n", card);
	printf("  -n,--name      control name to tap on\n");

	return 0;
}

static void value_change_event(snd_hctl_elem_t *helem)
{
	snd_ctl_elem_value_t *control;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_type_t type;
	snd_aes_iec958_t iec958;
	unsigned int idx, count;
	snd_ctl_elem_id_t *id;
	int err;

	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_value_alloca(&control);

	snd_hctl_elem_get_id(helem, id);

	err = snd_hctl_elem_info(helem, info);
	if (err < 0) {
		printf("Control %s snd_hctl_elem_info error: %s\n", card, snd_strerror(err));
		return;
	}

	count = snd_ctl_elem_info_get_count(info);
	type = snd_ctl_elem_info_get_type(info);
	if (!snd_ctl_elem_info_is_readable(info))
		return;

	err = snd_hctl_elem_read(helem, control);
	if (err < 0) {
		printf("Control %s element read error: %s\n", card, snd_strerror(err));
		return;
	}

	printf("'%s' (%s) changed: ", snd_ctl_elem_id_get_name(id),
	       snd_ctl_elem_type_name(type));

	for (idx = 0; idx < count; idx++) {
		if (idx > 0)
			printf(",");
		switch (type) {
		case SND_CTL_ELEM_TYPE_BOOLEAN:
			printf("%s", snd_ctl_elem_value_get_boolean(control, idx) ? "on" : "off");
			break;
		case SND_CTL_ELEM_TYPE_INTEGER:
			printf("%li", snd_ctl_elem_value_get_integer(control, idx));
			break;
		case SND_CTL_ELEM_TYPE_INTEGER64:
			printf("%lli", snd_ctl_elem_value_get_integer64(control, idx));
			break;
		case SND_CTL_ELEM_TYPE_ENUMERATED:
			printf("%u", snd_ctl_elem_value_get_enumerated(control, idx));
			break;
		case SND_CTL_ELEM_TYPE_BYTES:
			printf("0x%02x", snd_ctl_elem_value_get_byte(control, idx));
			break;
		case SND_CTL_ELEM_TYPE_IEC958:
			snd_ctl_elem_value_get_iec958(control, &iec958);
			printf("[AES0=0x%02x AES1=0x%02x AES2=0x%02x AES3=0x%02x]",
			       iec958.status[0], iec958.status[1],
			       iec958.status[2], iec958.status[3]);
			break;
		default:
			printf("?");
			break;
		}
	}

	printf("\n");
}

static int element_callback(snd_hctl_elem_t *elem, unsigned int mask)
{
	if (mask & SND_CTL_EVENT_MASK_VALUE)
		value_change_event(elem);
	return 0;
}

static void events_add(snd_hctl_elem_t *helem)
{
	snd_ctl_elem_id_t *id;

	snd_ctl_elem_id_alloca(&id);
	snd_hctl_elem_get_id(helem, id);
	if (!control_name || !strcmp(snd_ctl_elem_id_get_name(id), control_name)) {
		snd_hctl_elem_set_callback(helem, element_callback);
		num_events++;
	}
}

static int ctl_callback(snd_hctl_t *ctl, unsigned int mask,
		 snd_hctl_elem_t *elem)
{
	if (mask & SND_CTL_EVENT_MASK_ADD)
		events_add(elem);

	return 0;
}

#define SND_CARD_PREFIX		"hw"

#if defined(SND_LIB_VER)
 #if SND_LIB_VER(1, 2, 5) <= SND_LIB_VERSION
	#undef SND_CARD_PREFIX
	#define SND_CARD_PREFIX	"sysdefault"
 #endif
#endif

int main(int argc, char *argv[])
{
	static const struct option long_option[] = {
		{"help", 0, NULL, 'h'},
		{"card", 1, NULL, 'c'},
		{"device", 1, NULL, 'D'},
		{"control-name", 1, NULL, 'n'},
		{NULL, 0, NULL, 0},
	};
	snd_hctl_t *handle;
	int err, i;

	err = 0;
	while (1) {
		int c = getopt_long(argc, argv, "hc:D:n:", long_option, NULL);

		if (c < 0)
			break;
		switch (c) {
		case 'h':
			help();
			return 0;
		case 'c':
			i = snd_card_get_index(optarg);
			if (i >= 0 && i < 32) {
				sprintf(card, "%s:%i", SND_CARD_PREFIX, i);
			} else {
				printf("Invalid card number '%s'.\n", optarg);
				err++;
			}
			break;
		case 'D':
			strncpy(card, optarg, sizeof(card)-1);
			card[sizeof(card)-1] = '\0';
			break;
		case 'n':
			control_name = optarg;
			break;
		default:
			printf("Invalid switch or option -%c needs an argument.\n", c);
			err++;
		}
	}
	if (err)
		return -EINVAL;

	err = snd_hctl_open(&handle, card, 0);
	if (err < 0) {
		printf("Control %s open error: %s\n", card, snd_strerror(err));
		return err;
	}

	snd_hctl_set_callback(handle, ctl_callback);
	err = snd_hctl_load(handle);
	if (err < 0) {
		printf("Control %s hbuild error: %s\n", card, snd_strerror(err));
		goto out;
	}

	if (!num_events) {
		if (control_name)
			printf("Control '%s' not found on %s\n", control_name, card);
		else
			printf("No Control found for %s\n", card);

		err = -ENODEV;
		goto out;
	}

	printf("Listening on %s and '%s'...\n", card,
	       control_name ? control_name : "all controls");

	/* TODO: graceful exit */
	while (1) {
		int res = snd_hctl_wait(handle, -1);

		if (res >= 0) {
			res = snd_hctl_handle_events(handle);
			if (res < 0)
				printf("ERR: %s (%d)\n", snd_strerror(res), res);
		}
	}

out:
	snd_hctl_close(handle);
	return err;
}
