#include "sway/input/seat.h"
#include "sway/input/keyboard.h"
#include "sway/input/input-manager.h"
#include "log.h"

static void handle_keyboard_key(struct wl_listener *listener, void *data) {
	struct sway_keyboard *keyboard =
		wl_container_of(listener, keyboard, keyboard_key);
	struct wlr_seat *wlr_seat = keyboard->seat_device->sway_seat->wlr_seat;
	struct wlr_input_device *wlr_device =
		keyboard->seat_device->input_device->wlr_device;
	struct wlr_event_keyboard_key *event = data;
	wlr_seat_set_keyboard(wlr_seat, wlr_device);
	wlr_seat_keyboard_notify_key(wlr_seat, event->time_msec,
		event->keycode, event->state);
}

static void handle_keyboard_modifiers(struct wl_listener *listener,
		void *data) {
	struct sway_keyboard *keyboard =
		wl_container_of(listener, keyboard, keyboard_modifiers);
	struct wlr_seat *wlr_seat = keyboard->seat_device->sway_seat->wlr_seat;
	struct wlr_input_device *wlr_device =
		keyboard->seat_device->input_device->wlr_device;
	wlr_seat_set_keyboard(wlr_seat, wlr_device);
	wlr_seat_keyboard_notify_modifiers(wlr_seat);
}

struct sway_keyboard *sway_keyboard_create(struct sway_seat *seat,
		struct sway_seat_device *device) {
	struct sway_keyboard *keyboard =
		calloc(1, sizeof(struct sway_keyboard));
	if (!sway_assert(keyboard, "could not allocate sway keyboard")) {
		return NULL;
	}

	keyboard->seat_device = device;
	device->keyboard = keyboard;

	wl_list_init(&keyboard->keyboard_key.link);
	wl_list_init(&keyboard->keyboard_modifiers.link);

	return keyboard;
}

void sway_keyboard_configure(struct sway_keyboard *keyboard) {
	struct xkb_rule_names rules;
	memset(&rules, 0, sizeof(rules));
	struct input_config *input_config =
		keyboard->seat_device->input_device->config;
	struct wlr_input_device *wlr_device =
		keyboard->seat_device->input_device->wlr_device;

	if (input_config && input_config->xkb_layout) {
		rules.layout = input_config->xkb_layout;
	} else {
		rules.layout = getenv("XKB_DEFAULT_LAYOUT");
	}
	if (input_config && input_config->xkb_model) {
		rules.model = input_config->xkb_model;
	} else {
		rules.model = getenv("XKB_DEFAULT_MODEL");
	}

	if (input_config && input_config->xkb_options) {
		rules.options = input_config->xkb_options;
	} else {
		rules.options = getenv("XKB_DEFAULT_OPTIONS");
	}

	if (input_config && input_config->xkb_rules) {
		rules.rules = input_config->xkb_rules;
	} else {
		rules.rules = getenv("XKB_DEFAULT_RULES");
	}

	if (input_config && input_config->xkb_variant) {
		rules.variant = input_config->xkb_variant;
	} else {
		rules.variant = getenv("XKB_DEFAULT_VARIANT");
	}

	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!sway_assert(context, "cannot create XKB context")) {
		return;
	}

	struct xkb_keymap *keymap =
		xkb_keymap_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

	if (!keymap) {
		sway_log(L_DEBUG, "cannot configure keyboard: keymap does not exist");
		xkb_context_unref(context);
		return;
	}

	xkb_keymap_unref(keyboard->keymap);
	keyboard->keymap = keymap;
	wlr_keyboard_set_keymap(wlr_device->keyboard, keyboard->keymap);

	wlr_keyboard_set_repeat_info(wlr_device->keyboard, 25, 600);
	xkb_context_unref(context);
	struct wlr_seat *seat = keyboard->seat_device->sway_seat->wlr_seat;
	wlr_seat_set_keyboard(seat, wlr_device);

	wl_list_remove(&keyboard->keyboard_key.link);
	wl_signal_add(&wlr_device->keyboard->events.key, &keyboard->keyboard_key);
	keyboard->keyboard_key.notify = handle_keyboard_key;

	wl_list_remove(&keyboard->keyboard_modifiers.link);
	wl_signal_add( &wlr_device->keyboard->events.modifiers,
		&keyboard->keyboard_modifiers);
	keyboard->keyboard_modifiers.notify = handle_keyboard_modifiers;
}

void sway_keyboard_destroy(struct sway_keyboard *keyboard) {
	if (!keyboard) {
		return;
	}
	wl_list_remove(&keyboard->keyboard_key.link);
	wl_list_remove(&keyboard->keyboard_modifiers.link);
	free(keyboard);
}
