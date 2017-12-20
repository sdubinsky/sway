#include <string.h>
#include <strings.h>
#include "sway/commands.h"
#include "sway/input/input-manager.h"
#include "log.h"

struct cmd_results *cmd_seat(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "seat", EXPECTED_AT_LEAST, 2))) {
		return error;
	}

	if (config->reading && strcmp("{", argv[1]) == 0) {
		current_seat_config = new_seat_config(argv[0]);
		sway_log(L_DEBUG, "entering seat block: %s", current_seat_config->name);
		return cmd_results_new(CMD_BLOCK_SEAT, NULL, NULL);
	}

	if ((error = checkarg(argc, "seat", EXPECTED_AT_LEAST, 3))) {
		return error;
	}

	int argc_new = argc-2;
	char **argv_new = argv+2;

	struct cmd_results *res;
	current_seat_config = new_seat_config(argv[0]);
	if (strcasecmp("attach", argv[1]) == 0) {
		res = seat_cmd_attach(argc_new, argv_new);
	} else if (strcasecmp("fallback", argv[1]) == 0) {
		res = seat_cmd_fallback(argc_new, argv_new);
	} else {
		res = cmd_results_new(CMD_INVALID, "seat <name>", "Unknown command %s", argv[1]);
	}
	current_seat_config = NULL;
	return res;
}
